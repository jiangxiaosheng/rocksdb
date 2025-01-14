//  Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).
//
// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "file/writable_file_writer.h"

#include <algorithm>
#include <mutex>
#include <iostream>
#include <regex>

#include "db/version_edit.h"
#include "monitoring/histogram.h"
#include "monitoring/iostats_context_imp.h"
#include "port/port.h"
#include "test_util/sync_point.h"
#include "util/random.h"
#include "util/rate_limiter.h"

namespace ROCKSDB_NAMESPACE {
IOStatus WritableFileWriter::Append(const Slice& data) {
  const char* src = data.data();
  size_t left = data.size();
  IOStatus s;
  pending_sync_ = true;

  TEST_KILL_RANDOM("WritableFileWriter::Append:0",
                   rocksdb_kill_odds * REDUCE_ODDS2);

  // Calculate the checksum of appended data
  UpdateFileChecksum(data);

  {
    IOSTATS_TIMER_GUARD(prepare_write_nanos);
    TEST_SYNC_POINT("WritableFileWriter::Append:BeforePrepareWrite");
    writable_file_->PrepareWrite(static_cast<size_t>(GetFileSize()), left,
                                 IOOptions(), nullptr);
  }

  // See whether we need to enlarge the buffer to avoid the flush
  if (buf_.Capacity() - buf_.CurrentSize() < left) {
    for (size_t cap = buf_.Capacity();
         cap < max_buffer_size_;  // There is still room to increase
         cap *= 2) {
      // See whether the next available size is large enough.
      // Buffer will never be increased to more than max_buffer_size_.
      size_t desired_capacity = std::min(cap * 2, max_buffer_size_);
      if (desired_capacity - buf_.CurrentSize() >= left ||
          (use_direct_io() && desired_capacity == max_buffer_size_)) {
        buf_.AllocateNewBuffer(desired_capacity, true);
        break;
      }
    }
  }

  // Flush only when buffered I/O
  if (!use_direct_io() && (buf_.Capacity() - buf_.CurrentSize()) < left) {
    if (buf_.CurrentSize() > 0) {
      s = Flush();
      if (!s.ok()) {
        return s;
      }
    }
    assert(buf_.CurrentSize() == 0);
  }

  // We never write directly to disk with direct I/O on.
  // or we simply use it for its original purpose to accumulate many small
  // chunks
  if (use_direct_io() || (buf_.Capacity() >= left)) {
    while (left > 0) {
      size_t appended = buf_.Append(src, left);
      left -= appended;
      src += appended;

      if (left > 0) {
        s = Flush();
        if (!s.ok()) {
          break;
        }
      }
    }
  } else {
    // Writing directly to file bypassing the buffer
    assert(buf_.CurrentSize() == 0);
    s = WriteBuffered(src, left);
  }

  TEST_KILL_RANDOM("WritableFileWriter::Append:1", rocksdb_kill_odds);
  if (s.ok()) {
    filesize_ += data.size();
    // std::regex sst_regex(".*.sst");
    // if (std::regex_match(file_name_, sst_regex)){
    //   std::cout << "[Append]: " << file_name_ << " " << filesize_ << ", adding: " << data.size() << "\n";
    // }
  }
  return s;
}

IOStatus WritableFileWriter::Pad(const size_t pad_bytes) {
  assert(pad_bytes < kDefaultPageSize);
  size_t left = pad_bytes;
  size_t cap = buf_.Capacity() - buf_.CurrentSize();

  // Assume pad_bytes is small compared to buf_ capacity. So we always
  // use buf_ rather than write directly to file in certain cases like
  // Append() does.
  while (left) {
    size_t append_bytes = std::min(cap, left);
    buf_.PadWith(append_bytes, 0);
    left -= append_bytes;
    if (left > 0) {
      IOStatus s = Flush();
      if (!s.ok()) {
        return s;
      }
    }
    cap = buf_.Capacity() - buf_.CurrentSize();
  }
  pending_sync_ = true;
  filesize_ += pad_bytes;
  return IOStatus::OK();
}

IOStatus WritableFileWriter::Close() {
  // Do not quit immediately on failure the file MUST be closed
  IOStatus s;

  // Possible to close it twice now as we MUST close
  // in __dtor, simply flushing is not enough
  // Windows when pre-allocating does not fill with zeros
  // also with unbuffered access we also set the end of data.
  if (writable_file_.get() == nullptr) {
    return s;
  }

  s = Flush();  // flush cache to OS

  IOStatus interim;
  // In direct I/O mode we write whole pages so
  // we need to let the file know where data ends.
  if (use_direct_io()) {
    {
#ifndef ROCKSDB_LITE
      FileOperationInfo::StartTimePoint start_ts;
      if (ShouldNotifyListeners()) {
        start_ts = FileOperationInfo::StartNow();
      }
#endif
      interim = writable_file_->Truncate(filesize_, IOOptions(), nullptr);
#ifndef ROCKSDB_LITE
      if (ShouldNotifyListeners()) {
        auto finish_ts = FileOperationInfo::FinishNow();
        NotifyOnFileTruncateFinish(start_ts, finish_ts, s);
      }
#endif
    }
    if (interim.ok()) {
      {
#ifndef ROCKSDB_LITE
        FileOperationInfo::StartTimePoint start_ts;
        if (ShouldNotifyListeners()) {
          start_ts = FileOperationInfo::StartNow();
        }
#endif
        interim = writable_file_->Fsync(IOOptions(), nullptr);
#ifndef ROCKSDB_LITE
        if (ShouldNotifyListeners()) {
          auto finish_ts = FileOperationInfo::FinishNow();
          NotifyOnFileSyncFinish(start_ts, finish_ts, s,
                                 FileOperationType::kFsync);
        }
#endif
      }
    }
    if (!interim.ok() && s.ok()) {
      s = interim;
    }
  }

  TEST_KILL_RANDOM("WritableFileWriter::Close:0", rocksdb_kill_odds);
  {
#ifndef ROCKSDB_LITE
    FileOperationInfo::StartTimePoint start_ts;
    if (ShouldNotifyListeners()) {
      start_ts = FileOperationInfo::StartNow();
    }
#endif
    interim = writable_file_->Close(IOOptions(), nullptr);
#ifndef ROCKSDB_LITE
    if (ShouldNotifyListeners()) {
      auto finish_ts = FileOperationInfo::FinishNow();
      NotifyOnFileCloseFinish(start_ts, finish_ts, s);
    }
#endif
  }
  if (!interim.ok() && s.ok()) {
    s = interim;
  }

  writable_file_.reset();
  TEST_KILL_RANDOM("WritableFileWriter::Close:1", rocksdb_kill_odds);

  if (s.ok() && checksum_generator_ != nullptr && !checksum_finalized_) {
    checksum_generator_->Finalize();
    checksum_finalized_ = true;
  }

  return s;
}

// write out the cached data to the OS cache or storage if direct I/O
// enabled
IOStatus WritableFileWriter::Flush() {
  IOStatus s;
  TEST_KILL_RANDOM("WritableFileWriter::Flush:0",
                   rocksdb_kill_odds * REDUCE_ODDS2);

  if (buf_.CurrentSize() > 0) {
    if (use_direct_io()) {
#ifndef ROCKSDB_LITE
      if (pending_sync_) {
        // std::cout << " direct write \n";
        s = WriteDirect();
      }
#endif  // !ROCKSDB_LITE
    } else {
      s = WriteBuffered(buf_.BufferStart(), buf_.CurrentSize());
    }
    if (!s.ok()) {
      return s;
    }
  }

  {
#ifndef ROCKSDB_LITE
    FileOperationInfo::StartTimePoint start_ts;
    if (ShouldNotifyListeners()) {
      start_ts = FileOperationInfo::StartNow();
    }
#endif
    s = writable_file_->Flush(IOOptions(), nullptr);
#ifndef ROCKSDB_LITE
    if (ShouldNotifyListeners()) {
      auto finish_ts = std::chrono::steady_clock::now();
      NotifyOnFileFlushFinish(start_ts, finish_ts, s);
    }
#endif
  }

  if (!s.ok()) {
    return s;
  }

  // sync OS cache to disk for every bytes_per_sync_
  // TODO: give log file and sst file different options (log
  // files could be potentially cached in OS for their whole
  // life time, thus we might not want to flush at all).

  // We try to avoid sync to the last 1MB of data. For two reasons:
  // (1) avoid rewrite the same page that is modified later.
  // (2) for older version of OS, write can block while writing out
  //     the page.
  // Xfs does neighbor page flushing outside of the specified ranges. We
  // need to make sure sync range is far from the write offset.
  if (!use_direct_io() && bytes_per_sync_) {
    const uint64_t kBytesNotSyncRange =
        1024 * 1024;                                // recent 1MB is not synced.
    const uint64_t kBytesAlignWhenSync = 4 * 1024;  // Align 4KB.
    if (filesize_ > kBytesNotSyncRange) {
      uint64_t offset_sync_to = filesize_ - kBytesNotSyncRange;
      offset_sync_to -= offset_sync_to % kBytesAlignWhenSync;
      assert(offset_sync_to >= last_sync_size_);
      if (offset_sync_to > 0 &&
          offset_sync_to - last_sync_size_ >= bytes_per_sync_) {
        s = RangeSync(last_sync_size_, offset_sync_to - last_sync_size_);
        last_sync_size_ = offset_sync_to;
      }
    }
  }

  return s;
}

std::string WritableFileWriter::GetFileChecksum() {
  if (checksum_generator_ != nullptr) {
    assert(checksum_finalized_);
    return checksum_generator_->GetChecksum();
  } else {
    return kUnknownFileChecksum;
  }
}

const char* WritableFileWriter::GetFileChecksumFuncName() const {
  if (checksum_generator_ != nullptr) {
    return checksum_generator_->Name();
  } else {
    return kUnknownFileChecksumFuncName;
  }
}

IOStatus WritableFileWriter::Sync(bool use_fsync) {
  IOStatus s = Flush();
  if (!s.ok()) {
    return s;
  }
  TEST_KILL_RANDOM("WritableFileWriter::Sync:0", rocksdb_kill_odds);
  if (!use_direct_io() && pending_sync_) {
    s = SyncInternal(use_fsync);
    if (!s.ok()) {
      return s;
    }
  }
  TEST_KILL_RANDOM("WritableFileWriter::Sync:1", rocksdb_kill_odds);
  pending_sync_ = false;
  return IOStatus::OK();
}

IOStatus WritableFileWriter::SyncWithoutFlush(bool use_fsync) {
  if (!writable_file_->IsSyncThreadSafe()) {
    return IOStatus::NotSupported(
        "Can't WritableFileWriter::SyncWithoutFlush() because "
        "WritableFile::IsSyncThreadSafe() is false");
  }
  TEST_SYNC_POINT("WritableFileWriter::SyncWithoutFlush:1");
  IOStatus s = SyncInternal(use_fsync);
  TEST_SYNC_POINT("WritableFileWriter::SyncWithoutFlush:2");
  return s;
}

IOStatus WritableFileWriter::SyncInternal(bool use_fsync) {
  IOStatus s;
  IOSTATS_TIMER_GUARD(fsync_nanos);
  TEST_SYNC_POINT("WritableFileWriter::SyncInternal:0");
  auto prev_perf_level = GetPerfLevel();
  IOSTATS_CPU_TIMER_GUARD(cpu_write_nanos, env_);
#ifndef ROCKSDB_LITE
  FileOperationInfo::StartTimePoint start_ts;
  if (ShouldNotifyListeners()) {
    start_ts = FileOperationInfo::StartNow();
  }
#endif
  if (use_fsync) {
    s = writable_file_->Fsync(IOOptions(), nullptr);
  } else {
    s = writable_file_->Sync(IOOptions(), nullptr);
  }
#ifndef ROCKSDB_LITE
  if (ShouldNotifyListeners()) {
    auto finish_ts = std::chrono::steady_clock::now();
    NotifyOnFileSyncFinish(
        start_ts, finish_ts, s,
        use_fsync ? FileOperationType::kFsync : FileOperationType::kSync);
  }
#endif
  SetPerfLevel(prev_perf_level);
  return s;
}

IOStatus WritableFileWriter::RangeSync(uint64_t offset, uint64_t nbytes) {
  IOSTATS_TIMER_GUARD(range_sync_nanos);
  TEST_SYNC_POINT("WritableFileWriter::RangeSync:0");
#ifndef ROCKSDB_LITE
  FileOperationInfo::StartTimePoint start_ts;
  if (ShouldNotifyListeners()) {
    start_ts = FileOperationInfo::StartNow();
  }
#endif
  IOStatus s = writable_file_->RangeSync(offset, nbytes, IOOptions(), nullptr);
#ifndef ROCKSDB_LITE
  if (ShouldNotifyListeners()) {
    auto finish_ts = std::chrono::steady_clock::now();
    NotifyOnFileRangeSyncFinish(offset, nbytes, start_ts, finish_ts, s);
  }
#endif
  return s;
}

// This method writes to disk the specified data and makes use of the rate
// limiter if available
IOStatus WritableFileWriter::WriteBuffered(const char* data, size_t size) {
  IOStatus s;
  assert(!use_direct_io());
  const char* src = data;
  size_t left = size;

  while (left > 0) {
    size_t allowed;
    if (rate_limiter_ != nullptr) {
      allowed = rate_limiter_->RequestToken(
          left, 0 /* alignment */, writable_file_->GetIOPriority(), stats_,
          RateLimiter::OpType::kWrite);
    } else {
      allowed = left;
    }

    {
      IOSTATS_TIMER_GUARD(write_nanos);
      TEST_SYNC_POINT("WritableFileWriter::Flush:BeforeAppend");

#ifndef ROCKSDB_LITE
      FileOperationInfo::StartTimePoint start_ts;
      uint64_t old_size = writable_file_->GetFileSize(IOOptions(), nullptr);
      if (ShouldNotifyListeners()) {
        start_ts = FileOperationInfo::StartNow();
        old_size = next_write_offset_;
      }
#endif
      {
        auto prev_perf_level = GetPerfLevel();
        IOSTATS_CPU_TIMER_GUARD(cpu_write_nanos, env_);
        s = writable_file_->Append(Slice(src, allowed), IOOptions(), nullptr);
        SetPerfLevel(prev_perf_level);
      }
#ifndef ROCKSDB_LITE
      if (ShouldNotifyListeners()) {
        auto finish_ts = std::chrono::steady_clock::now();
        NotifyOnFileWriteFinish(old_size, allowed, start_ts, finish_ts, s);
      }
#endif
      if (!s.ok()) {
        return s;
      }
    }

    IOSTATS_ADD(bytes_written, allowed);
    TEST_KILL_RANDOM("WritableFileWriter::WriteBuffered:0", rocksdb_kill_odds);

    left -= allowed;
    src += allowed;
  }
  buf_.Size(0);
  return s;
}

void WritableFileWriter::UpdateFileChecksum(const Slice& data) {
  if (checksum_generator_ != nullptr) {
    checksum_generator_->Update(data.data(), data.size());
  }
}

// This flushes the accumulated data in the buffer. We pad data with zeros if
// necessary to the whole page.
// However, during automatic flushes padding would not be necessary.
// We always use RateLimiter if available. We move (Refit) any buffer bytes
// that are left over the
// whole number of pages to be written again on the next flush because we can
// only write on aligned
// offsets.
#ifndef ROCKSDB_LITE
IOStatus WritableFileWriter::WriteDirect() {
  assert(use_direct_io());
  IOStatus s;
  const size_t alignment = buf_.Alignment();
  assert((next_write_offset_ % alignment) == 0);

  // Calculate whole page final file advance if all writes succeed
  size_t file_advance = TruncateToPageBoundary(alignment, buf_.CurrentSize());

  // Calculate the leftover tail, we write it here padded with zeros BUT we
  // will write
  // it again in the future either on Close() OR when the current whole page
  // fills out
  size_t leftover_tail = buf_.CurrentSize() - file_advance;

  // Round up and pad
  buf_.PadToAlignmentWith(0);

  const char* src = buf_.BufferStart();
  uint64_t write_offset = next_write_offset_;
  size_t left = buf_.CurrentSize();

  while (left > 0) {
    // Check how much is allowed
    size_t size;
    if (rate_limiter_ != nullptr) {
      size = rate_limiter_->RequestToken(left, buf_.Alignment(),
                                         writable_file_->GetIOPriority(),
                                         stats_, RateLimiter::OpType::kWrite);
    } else {
      size = left;
    }

    {
      IOSTATS_TIMER_GUARD(write_nanos);
      TEST_SYNC_POINT("WritableFileWriter::Flush:BeforeAppend");
      FileOperationInfo::StartTimePoint start_ts;
      if (ShouldNotifyListeners()) {
        start_ts = FileOperationInfo::StartNow();
      }
      // direct writes must be positional
      s = writable_file_->PositionedAppend(Slice(src, size), write_offset,
                                           IOOptions(), nullptr);
      if (ShouldNotifyListeners()) {
        auto finish_ts = std::chrono::steady_clock::now();
        NotifyOnFileWriteFinish(write_offset, size, start_ts, finish_ts, s);
      }
      if (!s.ok()) {
        buf_.Size(file_advance + leftover_tail);
        return s;
      }
    }

    IOSTATS_ADD(bytes_written, size);
    left -= size;
    src += size;
    write_offset += size;
    assert((next_write_offset_ % alignment) == 0);
  }

  if (s.ok()) {
    // Move the tail to the beginning of the buffer
    // This never happens during normal Append but rather during
    // explicit call to Flush()/Sync() or Close()
    buf_.RefitTail(file_advance, leftover_tail);
    // This is where we start writing next time which may or not be
    // the actual file size on disk. They match if the buffer size
    // is a multiple of whole pages otherwise filesize_ is leftover_tail
    // behind
    next_write_offset_ += file_advance;
  }
  return s;
}
#endif  // !ROCKSDB_LITE
}  // namespace ROCKSDB_NAMESPACE
