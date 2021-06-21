#pragma once

#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <string> 
#include <memory>
#include <nlohmann/json.hpp>
#include <unordered_map>

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/alarm.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include "rubble_kv_store.grpc.pb.h"
#include "sync_client.h"

#include "rocksdb/db.h"
#include "port/port_posix.h"
#include "port/port.h"
#include "db/version_edit.h"
#include "db/db_impl/db_impl.h"
#include "rocksdb/slice.h"
#include "rocksdb/options.h"
#include "util/aligned_buffer.h"
#include "file/read_write_util.h"
#include "logging/event_logger.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Channel;
using grpc::ClientContext;
using grpc::ServerReaderWriter;
using grpc::Status;

using rubble::RubbleKvStoreService;
using rubble::SyncRequest;
using rubble::SyncReply;

using json = nlohmann::json;

//implements the Sync rpc call, if using this, server is serving DoOp requests asynchronously
class SyncServiceImpl final : public  RubbleKvStoreService::WithAsyncMethod_DoOp<RubbleKvStoreService::Service> {
  public:
    explicit SyncServiceImpl(rocksdb::DB* db);

    ~SyncServiceImpl();
  
  // a streaming RPC used by the non-tail node to sync Version(view of sst files) states to the downstream node 
  Status Sync(ServerContext* context, 
              ServerReaderWriter<SyncReply, SyncRequest>* stream) override;


  private:

    // actually handle the SyncRequest
    void HandleSyncRequest(const SyncRequest* request, 
                            SyncReply* reply);
    // every time server accepts a Sync rpc, needs to reset a few variables
    void ResetStates();

    // parse the version edit json string to rocksdb::VersionEdit 
    rocksdb::VersionEdit ParseJsonStringToVersionEdit(const json& j_edit /* json version edit */);

    //called by secondary nodes to create a pool of preallocated ssts in rubble mode
    rocksdb::IOStatus CreateSstPool();

    // In a 3-node setting, if it's the second node in the chain it should also ship sst files it received from the primary/first node
    // to the tail/downstream node and also delete the ones that get deleted in the compaction
    // for non-head node, should update sst bit map
    // since second node's flush is disabled ,we should do the shipping here when it received Sync rpc call from the primary
    /**
     * @param edit The version edit received from the priamry 
     * 
     */
    rocksdb::IOStatus UpdateSstBitMapAndShipSstFiles(const rocksdb::VersionEdit& edit);

    // set the reply message according to the status
    void SetReplyMessage(SyncReply* reply);

    // db instance
    rocksdb::DB* db_ = nullptr;
    rocksdb::DBImpl* impl_ = nullptr;
    // db's mutex
    rocksdb::InstrumentedMutex* mu_;
    // db status after processing an operation
    rocksdb::Status s_;
    rocksdb::IOStatus ios_;

    // rocksdb's version set
    rocksdb::VersionSet* version_set_;

    rocksdb::ColumnFamilySet* column_family_set_;
    // db's default columnfamily data 
    rocksdb::ColumnFamilyData* default_cf_;
    // rocksdb internal immutable db options
    const rocksdb::ImmutableDBOptions* db_options_;
    // rocksdb internal immutable column family options
    const rocksdb::ImmutableCFOptions* ioptions_;
    const rocksdb::MutableCFOptions* cf_options_;
  
    // right now, just put all sst files under one path
    rocksdb::DbPath db_path_;

    // maintain a mapping between sst_number and slot_number
    // sst_bit_map_[i] = j means sst_file with number j occupies the i-th slot
    // secondary node will update it when received a Sync rpc call from the upstream node
    std::unordered_map<int, uint64_t> sst_bit_map_;
    
    rocksdb::FileSystem* fs_;

    std::atomic<uint64_t> log_apply_counter_{0};

    // client for making Sync rpc call to downstream node
    std::shared_ptr<SyncClient> sync_client_;

    // is rubble mode? If set to false, server runs a vanilla rocksdb
    bool is_rubble_ = false;
    bool is_head_ = false;
    bool is_tail_ = false;

    /* variables below are used for Sync method */
    // if true, means version edit received indicates a flush job
    bool is_flush_ = false; 
    // indicates if version edit corresponds to a trivial move compaction
    bool is_trivial_move_compaction_ = false;
    // number of added sst files
    int num_of_added_files_ = 0;
    // number of memtables get flushed in a flush job
    int batch_count_ = 0;
    // get the next file num of secondary, which is the maximum file number of the AddedFiles in the shipped vesion edit plus 1
    int next_file_num_ = 0;

    // id for a Sync Request, assign it to the reply id
    uint64_t request_id_;

    // files that get deleted in a full compaction
    std::vector<uint64_t> deleted_files_;

    std::unordered_map<uint64_t ,int> added_;
};