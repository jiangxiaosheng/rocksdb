#include "rubble_sync_server.h"
#include <atomic>
#include <thread>
#include "db/memtable.h"
#include <ctime>

void PrintStatus(RubbleKvServiceImpl *srv) {
  uint64_t r_now = 0, r_old = srv->r_op_counter_.load();
  uint64_t w_now = 0, w_old = srv->w_op_counter_.load();
  while (true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    time_t t = time(0);
    r_now = srv->r_op_counter_.load();
    w_now = srv->w_op_counter_.load();
    int mem_num = srv->GetCFD()->imm()->current()->GetMemlist().size();
    int queued_op = srv->QueuedOpNum();
    uint64_t mem_id = srv->GetCFD()->mem()->GetID();
    uint64_t num_entries = srv->GetCFD()->mem()->num_entries();
    uint64_t ops = srv->GetCFD()->mem()->num_operations();
    uint64_t data_size = srv->GetCFD()->mem()->get_data_size();
    std::cout << "[READ] " << r_now << " op " << (r_now- r_old) << " op/s "
              << "[WRITE] " << w_now << " op " << (w_now- w_old) << " op/s "
              << "mem_num " << mem_num << " queued_op " << queued_op << " "
              << "mem_id " << mem_id << " entries " << num_entries << " ops " << ops << " data " << data_size << " "
              << ctime(&t);
    r_old = r_now;
    w_old = w_now;
  }
}

RubbleKvServiceImpl::RubbleKvServiceImpl(rocksdb::DB* db)
      :db_(db), impl_(static_cast<rocksdb::DBImpl*> (db)), 
       mu_(impl_->mutex()),
       version_set_(impl_->TEST_GetVersionSet()),
       table_cache_(impl_->TEST_table_cache()),
       db_options_(version_set_->db_options()),
       logger_(db_options_->rubble_info_log),
       is_rubble_(db_options_->is_rubble),
       is_head_(db_options_->is_primary),
       is_tail_(db_options_->is_tail),
       piggyback_edits_(db_options_->piggyback_version_edits),
       edits_(db_options_->edits),
       db_path_(db_options_->db_paths.front()),
       sync_client_(db_options_->sync_client),
       column_family_set_(version_set_->GetColumnFamilySet()),
       default_cf_(column_family_set_->GetDefault()),
       ioptions_(default_cf_->ioptions()),
       cf_options_(default_cf_->GetCurrentMutableCFOptions()),
       fs_(ioptions_->fs),
       status_thread_(PrintStatus, this){
        status_thread_.detach();
        if(is_rubble_ && !is_head_){
          std::cout << "init sync_service --- is_rubble: " << is_rubble_ << "; is_head: " << is_head_ << std::endl;
          ios_ = CreateSstPool();
          if(!ios_.ok()){
            std::cout << "allocate sst pool failed :" << ios_.ToString() << std::endl;
            assert(false);
          }
          std::cout << "[secondary] sst pool allocation finished" << std::endl;
        }
        std::cout << "target address : " << db_options_->target_address << std::endl;
        if(db_options_->target_address != ""){
          channel_ = db_options_->channel;
          assert(channel_ != nullptr);
        }
    };

RubbleKvServiceImpl::~RubbleKvServiceImpl(){
    delete db_;
}

rocksdb::ColumnFamilyData* RubbleKvServiceImpl::GetCFD() {
  return default_cf_;
}

int RubbleKvServiceImpl::QueuedOpNum() {
  int num = 0;
  for (std::map< std::thread::id, std::map< uint64_t, std::queue<SingleOp*> >* >::iterator it = buffers_.begin();
    it != buffers_.end(); it++) {
      for (std::map<uint64_t, std::queue<SingleOp*>>::iterator itt = it->second->begin();
        itt != it->second->end(); itt++) {
          num += itt->second.size();
        }
    }
  return num;
}

static std::atomic<uint32_t> op_counter{0};
static std::atomic<uint32_t> reply_counter{0};
static std::atomic<uint32_t> thread_counter{0};
static std::unordered_map<std::thread::id, uint32_t> map;
static std::mutex mu;
static std::map<uint64_t, uint64_t> primary_op_cnt_map;
#define G_MEM_ARR_LEN 1024
static std::atomic<uint64_t> histogram[G_MEM_ARR_LEN];

Status RubbleKvServiceImpl::DoOp(ServerContext* context, 
              ServerReaderWriter<OpReply, Op>* stream) {
    // if(!op_counter_.load()){
    //     start_time_ = high_resolution_clock::now();
    // }

    // if(op_counter_.load() && op_counter_.load()%100000 == 0){
    //     end_time_ = high_resolution_clock::now();
    //     auto millisecs = std::chrono::duration_cast<std::chrono::milliseconds>(end_time_ - start_time_);
    //     // why this doesn't print anything?
    //     std::cout << "Throughput : handle 100000  in " << millisecs.count() << " milisecs\n";
    //     start_time_ = end_time_;
    // }

    /* chain replication */
    // Forward the request to the downstream node in the chain if it's not a tail node

    std::shared_ptr<Forwarder> forwarder = nullptr;
    // client for sending the reply back to the replicator
    std::shared_ptr<ReplyClient> reply_client = nullptr;
    if(!db_options_->is_tail) {
      std::cout << "init the forwarder" << "\n";
      forwarder = std::make_shared<Forwarder>(channel_);
      // std::cout << "thread: " << map[std::this_thread::get_id()] << " Forwarded " << op_counter_ << " ops" << std::endl;
    } else {
      // tail node should be responsible for sending the reply back to replicator
      // use the sync stream to write the reply back
      if(channel_ != nullptr) {
        std::cout << "init the reply client" << "\n";
        reply_client = std::make_shared<ReplyClient>(channel_);
      }
    }

    std::thread::id this_id = std::this_thread::get_id();
    // mu.lock();
    // if(map.find(this_id) == map.end()){
    //   map[this_id] = thread_counter.load();
    //   thread_counter.fetch_add(1);
    // }
    // mu.unlock();

    Op tmp_op;
    std::map<uint64_t, std::queue<SingleOp *>> op_buffer;
    buffers_[std::this_thread::get_id()] = &op_buffer;
    while (stream->Read(&tmp_op)){
      Op* request = new Op(tmp_op);
      // handle DoOp requestbt
      OpReply reply;
      HandleOp(request, &reply, op_buffer);

      // if there is version edits piggybacked in the DoOp request, apply those edits
      if(request->has_edits()){
        size_t size = request->edits_size();
        RUBBLE_LOG_INFO(logger_ , "[Tail] Got %u new version edits\n", static_cast<uint32_t>(size));
        // fprintf(stdout , "[Tail] Got %u new version edits, op_counter : %lu \n", static_cast<uint32_t>(size), op_counter_.load());
        rocksdb::InstrumentedMutexLock l(mu_);
        for(int i = 0; i < size; i++){
          ApplyVersionEdits(request->edits(i));
        }
        RUBBLE_LOG_INFO(logger_ , "[Tail] finishes version edits\n");
      }

      if(forwarder == nullptr) { /* tail */
        // RUBBLE_LOG_INFO(logger_, "[Tail] received %lu ops\n", op_counter_.load());
        // fprintf(stdout, "[Tail] received %lu ops\n", op_counter_.load());
        if(reply_client != nullptr) {
          // reply_counter.fetch_add(1);
          // std::cout << "Sent out " << reply_counter.load() << " opreplies \n";
          reply_client->SendReply(reply);
        }
      } else { /* non-tail */
        // RUBBLE_LOG_INFO(logger_, "[Primary] thread %u Forwarded %lu ops\n", map[this_id], op_counter_.load());
        // fprintf(stdout, "[Primary] thread %u Forwarded %lu ops\n", map[this_id], op_counter_.load());
        if(piggyback_edits_ && is_rubble_ && is_head_) {
          std::vector<std::string> edits;
          edits_->GetEdits(edits);
          // for (std::string e : edits) {
          //   std::cout << "[Primary] Edit " << e << std::endl;
          // }
          if(edits.size() != 0) {
            size_t size = edits.size();
            // RUBBLE_LOG_INFO(logger_, "[primary] Got %u new version edits, op counter : %lu \n", static_cast<uint32_t>(size) ,op_counter_.load()); 
            request->set_has_edits(true);
            for(const auto& edit : edits) {
              RUBBLE_LOG_INFO(logger_, "Added Version Edit : %s \n", edit.c_str());
              request->add_edits(edit);
            }
            assert(request->edits_size() == size);
          } else {
            request->set_has_edits(false);
          }
        }
        forwarder->Forward(*request);
      }
    }
    std::cout << "end while loop with " << r_op_counter_.load() << " read and " << w_op_counter_.load() << " write ops done\n";
    for (int i = 1; i < G_MEM_ARR_LEN; i++) {
      if (histogram[i].load() == 0) {
        break;
      }
      std::cout << i << " : " << histogram[i].load() << std::endl;
    }

    if (forwarder != nullptr) {
      forwarder->WritesDone();
    }

    if (reply_client != nullptr) {
      reply_client->WritesDone();
    }

    return Status::OK;
}

uint64_t RubbleKvServiceImpl::get_mem_id() {
  return default_cf_->mem()->GetID();
}

bool RubbleKvServiceImpl::is_ooo_write(SingleOp* singleOp) {
  if (!is_rubble_ || is_head_) {
    return false;
  }

  if (singleOp->type() == rubble::GET) {
    return false;
  }

  uint64_t id = singleOp->target_mem_id();
  if (id == 0 || id == get_mem_id()) {
    assert(!is_rubble_ || id != 0);
    return false;
  }

  return true;
}

bool RubbleKvServiceImpl::should_execute(uint64_t target_mem_id) {
  rocksdb::MemTable* cur_mem = default_cf_->mem();
  uint64_t cur_mem_id = cur_mem->GetID();

  // 1. the request is for the current memtable
  if (target_mem_id == cur_mem_id) {
    return true;
  }

  // 2. the request is for the next memtable, but all requests for
  // the current one have been executed, so we need to execute it
  // to trigger SwitchMemTable
  if (target_mem_id == cur_mem_id + 1 &&
      cur_mem->num_target_op() != 0 &&
      cur_mem->num_operations() == cur_mem->num_target_op()) {
    std::cout << "trigger SwitchMemTable " << cur_mem_id << " by executing request for " << target_mem_id << std::endl;
    return true;
  }

  return false;
}

void RubbleKvServiceImpl::HandleOp(Op* op, OpReply* reply, std::map<uint64_t, std::queue<SingleOp*>>& op_buffer) {
  assert(op->ops_size() > 0);
  assert(reply->replies_size() == 0);

  reply->set_client_idx(op->client_idx());
  reply->add_time(op->time(0));

  batch_counter_.fetch_add(1);
  for (int i = 0; i < op->ops_size(); i++) {
    SingleOp* singleOp = op->mutable_ops(i);
    uint64_t id = singleOp->target_mem_id();
    if (!is_ooo_write(singleOp)) {
      HandleSingleOp(singleOp, reply);
    } else {
      rocksdb::MemTable* mem = default_cf_->mem();
      
      if (singleOp->mem_op_cnt() != 0) {
        uint64_t target_mem = singleOp->target_mem_id();
        uint64_t switched_mem = target_mem - 1;
        uint64_t target_op_cnt = singleOp->mem_op_cnt();
        std::cout << "received mem_op_cnt for mem " << switched_mem << " target_op_cnt " << target_op_cnt << std::endl;

        rocksdb::g_mem_id_arr[switched_mem % G_MEM_ARR_LEN] = switched_mem;
        rocksdb::g_mem_op_cnt_arr[switched_mem % G_MEM_ARR_LEN] = target_op_cnt;

        if (mem->GetID() == switched_mem) {
          std::cout << "set " << mem->GetID() << " 's target_op to " << target_op_cnt << std::endl;
          mem->set_num_target_op(target_op_cnt);
        }
      }

      op_buffer[id].emplace(singleOp);
      assert(id == singleOp->target_mem_id());
    }

    for (std::map<uint64_t, std::queue<SingleOp*>>::iterator it = op_buffer.begin();
      it != op_buffer.end(); it++) {
        id = it->first;
        if (should_execute(id)) {
          while (!op_buffer[id].empty()) {
            HandleSingleOp(op_buffer[id].front(), reply);
            op_buffer[id].pop();
          }
        }
      }
  }
}

void RubbleKvServiceImpl::HandleSingleOp(SingleOp* singleOp, OpReply* reply) {
  rocksdb::Status s;
  std::string value;
  SingleOpReply* singleOpReply;

  switch (singleOp->type()) {
    case rubble::GET:
      assert(is_tail_);
      s = db_->Get(rocksdb::ReadOptions(), singleOp->key(), &value);
      r_op_counter_.fetch_add(1);
      if (!s.ok()){
        RUBBLE_LOG_ERROR(logger_, "Get Failed : %s \n", s.ToString().c_str());
        // assert(false);
      }
      
      singleOpReply = reply->add_replies();
      singleOpReply->set_id(singleOp->id());
      singleOpReply->set_key(singleOp->key());
      singleOpReply->set_type(rubble::GET);
      singleOpReply->set_status(s.ToString());
      if (s.ok()) {
        singleOpReply->set_ok(true);
        singleOpReply->set_value(value);
      } else {
        singleOpReply->set_ok(false);
      }
      break;

    case rubble::PUT:
      s = db_->Put(rocksdb::WriteOptions(), singleOp->key(), singleOp->value());
      assert(s.get_target_mem_id() != 0);
      w_op_counter_.fetch_add(1);
      if (!s.ok()) {
        RUBBLE_LOG_ERROR(logger_, "Put Failed : %s \n", s.ToString().c_str());
        assert(false);
      }

      if (is_head_) {
        assert(singleOp->target_mem_id() == 0);
        singleOp->set_target_mem_id(s.get_target_mem_id());
        histogram[singleOp->target_mem_id()].fetch_add(1);
        // std::cout << "id: " << singleOp->target_mem_id() << std::endl;
        if (rocksdb::g_mem_op_cnt != 0 && s.get_target_mem_id() == rocksdb::g_mem_id) {
          rocksdb::g_mem_op_cnt_mtx.lock();
          if (rocksdb::g_mem_op_cnt != 0 && s.get_target_mem_id() == rocksdb::g_mem_id) {
            std::cout << "set " << s.get_target_mem_id() - 1 << " 's mem_op_cnt " << rocksdb::g_mem_op_cnt << std::endl;
            singleOp->set_mem_op_cnt(rocksdb::g_mem_op_cnt);
            rocksdb::g_mem_op_cnt = 0;
            rocksdb::g_mem_id = 0;
          }
          rocksdb::g_mem_op_cnt_mtx.unlock();
        }
      }

      if (is_tail_) { 
        assert(s.get_target_mem_id() == get_mem_id() || s.get_target_mem_id() == get_mem_id() - 1);
        assert(singleOp->target_mem_id() == get_mem_id());
        histogram[singleOp->target_mem_id()].fetch_add(1);
        singleOpReply = reply->add_replies();
        singleOpReply->set_id(singleOp->id());
        singleOpReply->set_type(rubble::PUT);
        singleOpReply->set_key(singleOp->key());
        singleOpReply->set_status(s.ToString());
        if (s.ok()) { 
          singleOpReply->set_ok(true);
        } else {
          singleOpReply->set_ok(false);
        }  
      }
      break;

    case rubble::UPDATE:
      s = db_->Get(rocksdb::ReadOptions(), singleOp->key(), &value);
      r_op_counter_.fetch_add(1);
      if (!s.ok()) {
        RUBBLE_LOG_ERROR(logger_, "Get Failed : %s \n", s.ToString().c_str());
        // assert(false);
      }

      s = db_->Put(rocksdb::WriteOptions(), singleOp->key(), singleOp->value());
      w_op_counter_.fetch_add(1);
      if (!s.ok()) {
        RUBBLE_LOG_ERROR(logger_, "Put Failed : %s \n", s.ToString().c_str());
        assert(false);
      }

      if (is_head_) {
        singleOp->set_target_mem_id(s.get_target_mem_id());
        // std::cout << "id: " << singleOp->target_mem_id() << std::endl;
        if (rocksdb::g_mem_op_cnt != 0) {
          rocksdb::g_mem_op_cnt_mtx.lock();
          if (rocksdb::g_mem_op_cnt != 0) {
            std::cout << "set mem_op_cnt " << rocksdb::g_mem_op_cnt << std::endl;
            singleOp->set_mem_op_cnt(rocksdb::g_mem_op_cnt);
            rocksdb::g_mem_op_cnt = 0;
          }
          rocksdb::g_mem_op_cnt_mtx.unlock();
        }
      }

      if (is_tail_) {
        singleOpReply = reply->add_replies();
        singleOpReply->set_id(singleOp->id());
        singleOpReply->set_key(singleOp->key());
        singleOpReply->set_type(rubble::UPDATE);
        singleOpReply->set_status(s.ToString());
        if (s.ok()) {
          singleOpReply->set_ok(true);
          singleOpReply->set_value(value);
        } else {
          singleOpReply->set_ok(false);
        }
      }
      break;

    default:
      std::cerr << "Unsupported Operation \n";
      break;
  }
}

// a streaming RPC used by the non-tail node to sync Version(view of sst files) states to the downstream node 
Status RubbleKvServiceImpl::Sync(ServerContext* context, 
              ServerReaderWriter<SyncReply, SyncRequest>* stream) {
    SyncRequest request;
    while (stream->Read(&request)) {
      SyncReply reply;
      // HandleSyncRequest(&request, &reply);
      std::string args = request.args();
      std::string message = ApplyVersionEdits(args);
      reply.set_message(message);
      stream->Write(reply);
    }
    return Status::OK;
}

void RubbleKvServiceImpl::HandleSyncRequest(const SyncRequest* request, 
                          SyncReply* reply) {
      
  std::string args = request->args();
  reply->set_message(ApplyVersionEdits(args));
}

// Update the secondary's states by applying the version edits
// and ship sst to the downstream node if necessary
std::string RubbleKvServiceImpl::ApplyVersionEdits(const std::string& args) {
    const json j_args = json::parse(args);
    bool is_flush = false;
    bool is_trivial_move = false;
    int batch_count = 0;

    if(j_args.contains("IsFlush")){
      assert(j_args["IsFlush"].get<bool>());
      batch_count = j_args["BatchCount"].get<int>();
      is_flush = true;
    }

    if(j_args.contains("IsTrivial")){
      is_trivial_move = true;
    }

    uint64_t version_edit_id = j_args["Id"].get<uint64_t>();
    uint64_t expected = version_edit_id_.load() + 1;
    
    uint64_t next_file_num = j_args["NextFileNum"].get<uint64_t>();
    rocksdb::autovector<rocksdb::VersionEdit*> edit_list;
    std::vector<rocksdb::VersionEdit> edits;
    for(const auto& edit_string : j_args["EditList"].get<std::vector<std::string>>()){  
      auto j_edit = json::parse(edit_string);
      auto edit = ParseJsonStringToVersionEdit(j_edit);
      // RUBBLE_LOG_INFO(logger_, "Got a new edit %s ", edit.DebugString().c_str());
      if(is_flush){
        edit.SetBatchCount(batch_count);
      }else if(is_trivial_move){
        edit.MarkTrivialMove();
      }
      edit.SetNextFile(next_file_num);
      edits.push_back(edit);      
    }
  
    if(version_edit_id != expected){
      RUBBLE_LOG_INFO(logger_, "Version Edit arrives out of order, expecting %lu, cache %lu \n", expected, version_edit_id);
      for(const auto& edit : edits){
       cached_edits_.insert({edit.GetEditNumber(), edit});
      }
      return "ok";
    }

    version_edit_id_.store(version_edit_id);
    ApplyOneVersionEdit(edits);
    edits.clear();
    if(cached_edits_.size() != 0){
      expected = version_edit_id + 1;
      int count = cached_edits_.count(expected);
      // auto it = cached_edits_.cbegin();
      while(count != 0){
        RUBBLE_LOG_INFO(logger_, "Got %d edits in cache, edit id : %lu \n", count, expected);
        for(int i = 0; i < count; i++){
          auto it = cached_edits_.cbegin();
          assert(it->first == expected);
          edits.push_back(it->second);
          cached_edits_.erase(it);
        }
        version_edit_id_.store(expected);
        ApplyOneVersionEdit(edits);
        edits.clear();
        expected++;
        count = cached_edits_.count(expected);
      }
    }

    return "ok";
}


std::string RubbleKvServiceImpl::ApplyOneVersionEdit(std::vector<rocksdb::VersionEdit>& edits) {
   
    bool is_flush = false;
    bool is_trivial_move = false;
    int batch_count = 0;
    if(edits.size() >= 2){
      for(const auto& edit : edits){
        assert(edit.IsFlush());
      }
    }
    auto& first_edit = edits[0];
    if(first_edit.IsFlush()){
      is_flush = true;
    }else if(first_edit.IsTrivialMove()){
      is_trivial_move = true;
    }

    batch_count += first_edit.GetBatchCount();
    uint64_t next_file_num = first_edit.GetNextFile();

    rocksdb::IOStatus ios;
    for(const auto& edit: edits){
      ios = UpdateSstViewAndShipSstFiles(edit);
      assert(ios.ok());
    }
     
    rocksdb::autovector<rocksdb::VersionEdit*> edit_list;
    for(auto& edit : edits){  
      edit_list.push_back(&edit);
    }

    const rocksdb::MutableCFOptions* cf_options = default_cf_->GetCurrentMutableCFOptions();
    rocksdb::autovector<rocksdb::ColumnFamilyData*> cfds;
    cfds.emplace_back(default_cf_);

    rocksdb::autovector<const rocksdb::MutableCFOptions*> mutable_cf_options_list;
    mutable_cf_options_list.emplace_back(cf_options);

    rocksdb::autovector<rocksdb::autovector<rocksdb::VersionEdit*>> edit_lists;
    edit_lists.emplace_back(edit_list);

    rocksdb::FSDirectory* db_directory = impl_->directories_.GetDbDir();
    rocksdb::MemTableList* imm = default_cf_->imm();
    
    uint64_t current_next_file_num = version_set_->current_next_file_number();
    // set secondary version set's next file num according to the primary's next_file_num_
    version_set_->FetchAddFileNumber(next_file_num - current_next_file_num);
    assert(version_set_->current_next_file_number() == next_file_num);

    log_apply_counter_.fetch_add(1);
    RUBBLE_LOG_INFO(logger_, " Accepting Sync %lu th times \n", log_apply_counter_.load());
    // Calling LogAndApply on the secondary
    rocksdb::Status s = version_set_->LogAndApply(cfds, mutable_cf_options_list, edit_lists, mu_,
                      db_directory);
    if(s.ok()){
      RUBBLE_LOG_INFO(logger_, "[Secondary] logAndApply succeeds \n");
    }else{
      RUBBLE_LOG_ERROR(logger_, "[Secondary] logAndApply failed : %s \n" , s.ToString().c_str());
      std::cerr << "[Secondary] logAndApply failed : " << s.ToString() << std::endl;
    }

    //Drop The corresponding MemTables in the Immutable MemTable List
    //If this version edit corresponds to a flush job
    if(is_flush){
      // creating a new verion after we applied the edit
      imm->InstallNewVersion();

      // All the later memtables that have the same filenum
      // are part of the same batch. They can be committed now.
      uint64_t mem_id = 1;  // how many memtables have been flushed.
      
      rocksdb::autovector<rocksdb::MemTable*> to_delete;
      if(s.ok() && !default_cf_->IsDropped()){

        rocksdb::SuperVersion* sv = default_cf_->GetSuperVersion();
        rocksdb::MemTableListVersion* current = imm->current();
        // assert(imm->current()->GetMemlist().size() >= batch_count_) ? 
        // This is not always the case, sometimes secondary has only one immutable memtable in the list, say ID 89,
        // while the primary has 2 immutable memtables, say 89 and 90, with a more latest one,
        // so should set the number_of_immutable_memtable_to_delete to be the minimum of batch count and immutable memlist size
        int imm_size = (int)current->GetMemlist().size();
        int num_of_imm_to_delete = std::min(batch_count, imm_size);
        RUBBLE_LOG_INFO(logger_ , "memlist size : %d, batch count : %d \n", imm_size ,  batch_count);
        // fprintf(stdout, "memlist size : %d, bacth count : %d ,num_of_imms_to_delete : %d \n", imm_size ,batch_count, num_of_imm_to_delete);
        int i = 0;
        while(num_of_imm_to_delete -- > 0){
          rocksdb::MemTable* m = current->GetMemlist().back();
          m->SetFlushCompleted(true);

          auto& edit = edit_list[i];
          auto& new_files = edit->GetNewFiles();
          m->SetFileNumber(new_files[0].second.fd.GetNumber()); 
          if(edit->GetBatchCount() == 1){
            i++;
          }
          
          RUBBLE_LOG_INFO(logger_,
                        "[%s] Level-0 commit table #%lu : memtable #%lu done",
                        default_cf_->GetName().c_str(), m->GetFileNumber(), mem_id);

          assert(m->GetFileNumber() > 0);
          /* drop the corresponding immutable memtable in the list if version edit corresponds to a flush */
          // according the code comment in the MemTableList class : "The memtables are flushed to L0 as soon as possible and in any order." 
          // as far as I observe, it's always the back of the imm memlist gets flushed first, which is the earliest memtable
          // so here we always drop the memtable in the back of the list
          current->RemoveLast(sv->GetToDelete());

          imm->SetNumFlushNotStarted(current->GetMemlist().size());
          imm->UpdateCachedValuesFromMemTableListVersion();
          imm->ResetTrimHistoryNeeded();
          ++mem_id;
        }
      }else {
        //TODO : Commit Failed For Some reason, need to reset state
        std::cout << "[Secondary] Flush logAndApply Failed : " << s.ToString() << std::endl;
      }

      imm->SetCommitInProgress(false);
      RUBBLE_LOG_INFO(logger_, "After RemoveLast : ( ImmutableList size : %u ) \n", static_cast<uint32_t>(imm->current()->GetMemlist().size()));
      // RUBBLE_LOG_INFO(logger_, "After RemoveLast : ( ImmutableList : %s ) \n", json::parse(imm->DebugJson()).dump(4).c_str());
      int size = static_cast<int> (imm->current()->GetMemlist().size());
      // std::cout << " memlist size : " << size << " , num_flush_not_started : " << imm->GetNumFlushNotStarted() << std::endl;
    } else { // It is either a trivial move compaction or a full compaction
      if(is_trivial_move){
        if(!s.ok()){
          std::cout << "[Secondary] Trivial Move LogAndApply Failed : " << s.ToString() << std::endl;
        }else{
          // std::cout << "[Secondary] Trivial Move LogAndApply Succeeds \n";
        }
      }else{ // it's a full compaction
        // std::cout << "[Secondary] Full compaction LogAndApply status : " << s.ToString() << std::endl;
      }
    }

    assert(s.ok());
    std::string ret = "ok";
    return ret;
    // SetReplyMessage(reply, s, is_flush, is_trivial_move);
}


// parse the version edit json string to rocksdb::VersionEdit 
rocksdb::VersionEdit RubbleKvServiceImpl::ParseJsonStringToVersionEdit(const json& j_edit /* json version edit */){
    rocksdb::VersionEdit edit;

    assert(j_edit.contains("AddedFiles"));
    if(j_edit.contains("IsFlush")){ // means edit corresponds to a flush job
        edit.MarkFlush();
    }

    if(j_edit.contains("LogNumber")){
        edit.SetLogNumber(j_edit["LogNumber"].get<uint64_t>());
    }
    if(j_edit.contains("PrevLogNumber")){
        edit.SetPrevLogNumber(j_edit["PrevLogNumber"].get<uint64_t>());
    }
    
    // right now, just use one column family(the default one)
    edit.SetColumnFamily(0);

    assert(j_edit.contains("EditNumber"));
    edit.SetEditNumber(j_edit["EditNumber"].get<uint64_t>());
    
    for(const auto& j_added_file : j_edit["AddedFiles"].get<std::vector<json>>()){
        // std::cout << j_added_file.dump(4) << std::endl;
        assert(!j_added_file["SmallestUserKey"].is_null());
        assert(!j_added_file["SmallestSeqno"].is_null());
        // TODO: should decide ValueType according to the info in the received version edit 
        // basically pass the ValueType of the primary's version Edit's smallest/largest InterKey's ValueType
        rocksdb::InternalKey smallest(rocksdb::Slice(j_added_file["SmallestUserKey"].get<std::string>()), 
                                      j_added_file["SmallestSeqno"].get<uint64_t>(),
                                      rocksdb::ValueType::kTypeValue);

        assert(smallest.Valid());

        uint64_t smallest_seqno = j_added_file["SmallestSeqno"].get<uint64_t>();
        
        assert(!j_added_file["LargestUserKey"].is_null());
        assert(!j_added_file["LargestSeqno"].is_null());
        rocksdb::InternalKey largest(rocksdb::Slice(j_added_file["LargestUserKey"].get<std::string>()), 
                                      j_added_file["LargestSeqno"].get<uint64_t>(),
                                      rocksdb::ValueType::kTypeValue);

        assert(largest.Valid());

        uint64_t largest_seqno = j_added_file["LargestSeqno"].get<uint64_t>();
        int level = j_added_file["Level"].get<int>();
        uint64_t file_num = j_added_file["FileNumber"].get<uint64_t>();

        if(j_added_file.contains("Slot")){
          edit.TrackSlot(file_num, j_added_file["Slot"].get<int>());
        }
        uint64_t file_size = j_added_file["FileSize"].get<uint64_t>();

        std::string file_checksum = j_added_file["FileChecksum"].get<std::string>();
        std::string file_checksum_func_name = j_added_file["FileChecksumFuncName"].get<std::string>();
        uint64_t file_creation_time = j_added_file["FileCreationTime"].get<uint64_t>();
        uint64_t oldest_ancester_time = j_added_file["OldestAncesterTime"].get<uint64_t>();

        const rocksdb::FileMetaData meta(file_num, 0/* path_id shoule be 0*/,
                                        file_size, 
                                        smallest, largest, 
                                        smallest_seqno, largest_seqno,
                                        false, 
                                        rocksdb::kInvalidBlobFileNumber,
                                        oldest_ancester_time,
                                        file_creation_time,
                                        file_checksum, 
                                        file_checksum_func_name);

        edit.AddFile(level, meta);
    }

    if(j_edit.contains("DeletedFiles")){
        for(const auto& j_delete_file : j_edit["DeletedFiles"].get<std::vector<json>>()){
            edit.DeleteFile(j_delete_file["Level"].get<int>(), j_delete_file["FileNumber"].get<uint64_t>());
        }   
    }
    return std::move(edit);
}

//called by secondary nodes to create a pool of preallocated ssts in rubble mode
rocksdb::IOStatus RubbleKvServiceImpl::CreateSstPool(){
    const std::string sst_dir = db_path_.path;
    uint64_t target_size = db_path_.target_size;
    // size_t write_buffer_size = cf_options_->write_buffer_size;
    uint64_t target_file_size_base = cf_options_->target_file_size_base;
    assert((target_file_size_base % (1 << 20)) == 0);
    //assume the target_file_size_base is an integer multiple of 1MB
    // use one more MB because of the footer, and pad to the buffer_size
    uint64_t buffer_size = target_file_size_base + db_options_->sst_pad_len;
    uint64_t big_buffer_size = 2*target_file_size_base + db_options_->sst_pad_len;

    std::cout << "sst file size : " << buffer_size << std::endl;

    rocksdb::AlignedBuffer buf;
    rocksdb::IOStatus s;
    
    int max_num_mems_to_flush = db_options_->max_num_mems_in_flush;
    int pool_size = db_options_->preallocated_sst_pool_size;
    int big_sst_pool_size = 10;
    for (int i = 1; i <= pool_size + (max_num_mems_to_flush - 1)* big_sst_pool_size; i++) {
        std::string sst_num = std::to_string(i);
        // rocksdb::WriteStringToFile(fs_, rocksdb::Slice(std::string(buffer_size, 'c')), sst_dir + "/" + fname, true);
        std::string sst_name = sst_dir + "/" + sst_num;
        s = fs_->FileExists(sst_name, rocksdb::IOOptions(), nullptr);
        if(!s.ok()) {
            std::unique_ptr<rocksdb::FSWritableFile> file;
            rocksdb::EnvOptions soptions;
            soptions.use_direct_writes = true;
            s = fs_->NewWritableFile(sst_name, soptions, &file, nullptr);
            if (!s.ok()) {
                return s;
            }
            if(i == 1){
                buf.Alignment(file->GetRequiredBufferAlignment());
                buf.AllocateNewBuffer(buffer_size);
                buf.PadWith(buffer_size, 'c');
            }else if( i > pool_size && ( ((i - pool_size) % big_sst_pool_size) == 1) ){
                int times = (i - pool_size) / big_sst_pool_size + 2;
                std::cout << " times : " << times << std::endl;
                buffer_size = times * target_file_size_base + db_options_->sst_pad_len;
                buf.Alignment(file->GetRequiredBufferAlignment());
                buf.AllocateNewBuffer(buffer_size);
                buf.PadWith(buffer_size - buf.CurrentSize(), 'c');
            }

            s = file->Append(rocksdb::Slice(buf.BufferStart()), rocksdb::IOOptions(), nullptr);
            if (s.ok()) {
                s = file->Sync(rocksdb::IOOptions(), nullptr);
            }
            if (!s.ok()) {
                fs_->DeleteFile(sst_name, rocksdb::IOOptions(), nullptr);
                return s;
            }
        }
    }
         
    std::cout << "allocated " << db_options_->preallocated_sst_pool_size << " sst slots in " << sst_dir << std::endl;
    return s;
}


// In a 3-node setting, if it's the second node in the chain it should also ship sst files it received from the primary/first node
// to the tail/downstream node and also delete the ones that get deleted in the compaction
// for non-head node, should update sst bit map
// since second node's flush is disabled ,we should do the shipping here when it received Sync rpc call from the primary
/**
 * @param edit The version edit received from the priamry 
 * 
 */
rocksdb::IOStatus RubbleKvServiceImpl::UpdateSstViewAndShipSstFiles(const rocksdb::VersionEdit& edit){
    if(edit.IsTrivialMove()){
      return rocksdb::IOStatus::OK();
    }
    rocksdb::IOStatus ios;
    for(const auto& new_file: edit.GetNewFiles()){
        const rocksdb::FileMetaData& meta = new_file.second;
        uint64_t file_num = meta.fd.GetNumber();
        // int sst_real = TakeOneAvailableSstSlot(new_file.first, meta);
        int sst_real = edit.GetSlot(file_num);
        
        // rocksdb::DirectReadKBytes(fs_, sst_real, 32, db_path_.path + "/");

        int len = std::to_string(file_num).length();
        std::string sst_file_name = std::string("000000").replace(6 - len, len, std::to_string(file_num)) + ".sst";
        std::string fname = rocksdb::TableFileName(ioptions_->cf_paths, file_num, meta.fd.GetPathId());
        // update secondary's view of sst files
        // std::cout << " Link " << sst_real << " to " << fname << std::endl;
        ios = fs_->LinkFile(db_path_.path + "/" + std::to_string(sst_real), fname, rocksdb::IOOptions(), nullptr);
        if(!ios.ok()){
            std::cout << ios.ToString() << std::endl;
            return ios;
        }else{
          RUBBLE_LOG_INFO(logger_, "[create new sst]: %s , linking to %d \n", sst_file_name.c_str(), sst_real);
          // std::cout << "[create new sst]: "  << sst_file_name << " , linking to " << sst_real << std::endl;
        }
        
        // tail node doesn't need to ship sst files
        if(!is_tail_){
            assert(db_options_->remote_sst_dir != "");
            std::string remote_sst_dir = db_options_->remote_sst_dir;
            if(remote_sst_dir[remote_sst_dir.length() - 1] != '/'){
                remote_sst_dir = remote_sst_dir + '/';
            }
            std::string remote_sst_fname = remote_sst_dir + std::to_string(sst_real);
            // maybe not ship the sst file here, instead ship after we finish the logAndApply..
            ios = rocksdb::CopySstFile(fs_, fname, remote_sst_fname, 0,  true);
            if (!ios.ok()){
                std::cerr << "[ File Ship Failed ] : " << meta.fd.GetNumber() << std::endl;
            }else {
                std::cout << "[ File Shipped ] : " << meta.fd.GetNumber() << std::endl;
            }
        }
    }
    
    // TODO: should delete file after logAndApply finishes and invalidated the block cache for the deleted files
    // delete the ssts(the hard link) that get deleted in a non-trivial compaction
    for(const auto& delete_file : edit.GetDeletedFiles()){
        uint64_t file_number = delete_file.second;
        std::string file_path = rocksdb::MakeTableFileName(ioptions_->cf_paths[0].path, file_number);

        std::string sst_file_name = file_path.substr((file_path.find_last_of("/") + 1));
        // std::string fname = db_path_.path + "/" + sst_file_name;
        ios = fs_->FileExists(file_path, rocksdb::IOOptions(), nullptr);
        if (ios.ok()){
            // Evict cache entry for the deleted file
            rocksdb::TableCache::Evict(table_cache_, file_number);
            // delete the symbolic link
            ios = fs_->DeleteFile(file_path, rocksdb::IOOptions(), nullptr); 
            if(ios.IsIOError()){
                std::cerr << "[ File Deletion Failed ]:" <<  file_path << std::endl;
            }else if(ios.ok()){
              RUBBLE_LOG_INFO(logger_,"[File Deletion ]: %s \n", sst_file_name.c_str());
              // std::cout << "[ File Deletion] : " << file_path << std::endl;
              // FreeSstSlot(delete_file.second);
            }
        }else {
            if (ios.IsNotFound()){
                std::cerr << "file :" << file_path << "does not exist \n";
            } else {
                std::cerr << "unknown error in sync_service_impl" << std::endl;
            }
        }
    }
    return ios;
}

// set the reply message according to the status
void RubbleKvServiceImpl::SetReplyMessage(SyncReply* reply,const rocksdb::Status& s, bool is_flush, bool is_trivial_move){
    rocksdb::JSONWriter jw;

    json j_reply;
    j_reply["Id"] = request_id_;
    if(s.ok()){
        jw << "Status" << "Ok";
        
        if(!is_flush && !is_trivial_move){
            jw << "Type" << "Full";
            jw << "Deleted";
            jw.StartArray();

            assert(!deleted_files_.empty());
            for (const auto& deleted_file : deleted_files_) {
                jw << deleted_file;
            }

            jw.EndArray();
        }else if(is_flush){
            jw << "Type" << "Flush";
        }else{
            jw << "Type" << "Trivial";
        }
        jw.EndObject();

        std::string message = jw.Get();
        j_reply["Message"] = message;
        // std::cout << message << std::endl;
        reply->set_message(j_reply.dump());
    
    }else{
        jw << "Status" << "Failed";
        jw << "Reason" << s.ToString();
        jw.EndObject();

        std::string message = jw.Get();
        j_reply["Message"] = message;

        // std::cout << "Message : " << j_reply.dump(4) << std::endl;
        reply->set_message(j_reply.dump());
    }
}