#pragma once

#include <vector>
#include <iostream>
#include <string>
#include <chrono>

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include "rubble_kv_store.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using grpc::ClientReaderWriter;

using rubble::RubbleKvStoreService;
using rubble::Reply;
using rubble::OpReply;
// using rubble::Op_OpType_Name;


// client class used by the tail node in the chain to send the true reply back to the replicator
class ReplyClient{
  public:
    ReplyClient(std::shared_ptr<Channel> channel)
        : stub_(RubbleKvStoreService::NewStub(channel)) {    
        stream_ = stub_->SendReply(&context_);
    };

    ~ReplyClient(){
      std::cout << "reply client destroyed\n";
    }

    // send the reply to the replicator
    void SendReply(const OpReply& reply){
      // std::cout << "Sent reply, size : "  << reply.reply_size() << std::endl;
      if (!stream_->Write(reply)) {
        stream_->WritesDone();
        Status s = stream_->Finish();
        std::cout << "sendReply fail!"
                  << " msg: " << s.error_message() 
                  << " detail: " << s.error_details() 
                  << " debug: " << context_.debug_error_string()
                  << " shard: " << shard_idx << " client: " << client_idx << std::endl;
        assert(false);
      }
      //  std::cout << "sendReply client on reply: " << reply.ok() << "\n";
    }

    void WritesDone() {
      stream_->WritesDone();
      stream_->Finish();
    }

    void set_idx(int s, int c) {
      shard_idx = s;
      client_idx = c;
    }
  
  private:
    int shard_idx = -1;
    int client_idx = -1;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context_;

    // The bidirectional,synchronous stream for sending/receiving messages.
    std::unique_ptr<ClientReaderWriter<OpReply, Reply>> stream_;
    // Out of the passed in Channel comes the stub, stored here, our view of the
    // server's exposed services.
    std::unique_ptr<RubbleKvStoreService::Stub> stub_ = nullptr;
};
