// Generated by the gRPC C++ plugin.
// If you make any local change, they will be lost.
// source: rubble_kv_store.proto
#ifndef GRPC_rubble_5fkv_5fstore_2eproto__INCLUDED
#define GRPC_rubble_5fkv_5fstore_2eproto__INCLUDED

#include "rubble_kv_store.pb.h"

#include <functional>
#include <grpc/impl/codegen/port_platform.h>
#include <grpcpp/impl/codegen/async_generic_service.h>
#include <grpcpp/impl/codegen/async_stream.h>
#include <grpcpp/impl/codegen/async_unary_call.h>
#include <grpcpp/impl/codegen/client_callback.h>
#include <grpcpp/impl/codegen/client_context.h>
#include <grpcpp/impl/codegen/completion_queue.h>
#include <grpcpp/impl/codegen/message_allocator.h>
#include <grpcpp/impl/codegen/method_handler.h>
#include <grpcpp/impl/codegen/proto_utils.h>
#include <grpcpp/impl/codegen/rpc_method.h>
#include <grpcpp/impl/codegen/server_callback.h>
#include <grpcpp/impl/codegen/server_callback_handlers.h>
#include <grpcpp/impl/codegen/server_context.h>
#include <grpcpp/impl/codegen/service_type.h>
#include <grpcpp/impl/codegen/status.h>
#include <grpcpp/impl/codegen/stub_options.h>
#include <grpcpp/impl/codegen/sync_stream.h>

namespace rubble {

class RubbleKvStoreService final {
 public:
  static constexpr char const* service_full_name() {
    return "rubble.RubbleKvStoreService";
  }
  class StubInterface {
   public:
    virtual ~StubInterface() {}
    // an internal rpc call used by the upstream (except the tail node) 
    // to synchronize states to the downstream nodes in the chain
    std::unique_ptr< ::grpc::ClientReaderWriterInterface< ::rubble::SyncRequest, ::rubble::SyncReply>> Sync(::grpc::ClientContext* context) {
      return std::unique_ptr< ::grpc::ClientReaderWriterInterface< ::rubble::SyncRequest, ::rubble::SyncReply>>(SyncRaw(context));
    }
    std::unique_ptr< ::grpc::ClientAsyncReaderWriterInterface< ::rubble::SyncRequest, ::rubble::SyncReply>> AsyncSync(::grpc::ClientContext* context, ::grpc::CompletionQueue* cq, void* tag) {
      return std::unique_ptr< ::grpc::ClientAsyncReaderWriterInterface< ::rubble::SyncRequest, ::rubble::SyncReply>>(AsyncSyncRaw(context, cq, tag));
    }
    std::unique_ptr< ::grpc::ClientAsyncReaderWriterInterface< ::rubble::SyncRequest, ::rubble::SyncReply>> PrepareAsyncSync(::grpc::ClientContext* context, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncReaderWriterInterface< ::rubble::SyncRequest, ::rubble::SyncReply>>(PrepareAsyncSyncRaw(context, cq));
    }
    // perform db operations (CRUD)
    std::unique_ptr< ::grpc::ClientReaderWriterInterface< ::rubble::Op, ::rubble::OpReply>> DoOp(::grpc::ClientContext* context) {
      return std::unique_ptr< ::grpc::ClientReaderWriterInterface< ::rubble::Op, ::rubble::OpReply>>(DoOpRaw(context));
    }
    std::unique_ptr< ::grpc::ClientAsyncReaderWriterInterface< ::rubble::Op, ::rubble::OpReply>> AsyncDoOp(::grpc::ClientContext* context, ::grpc::CompletionQueue* cq, void* tag) {
      return std::unique_ptr< ::grpc::ClientAsyncReaderWriterInterface< ::rubble::Op, ::rubble::OpReply>>(AsyncDoOpRaw(context, cq, tag));
    }
    std::unique_ptr< ::grpc::ClientAsyncReaderWriterInterface< ::rubble::Op, ::rubble::OpReply>> PrepareAsyncDoOp(::grpc::ClientContext* context, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncReaderWriterInterface< ::rubble::Op, ::rubble::OpReply>>(PrepareAsyncDoOpRaw(context, cq));
    }
    // used by the tail node to send the true rely to the replicator
    std::unique_ptr< ::grpc::ClientReaderWriterInterface< ::rubble::OpReply, ::rubble::Reply>> SendReply(::grpc::ClientContext* context) {
      return std::unique_ptr< ::grpc::ClientReaderWriterInterface< ::rubble::OpReply, ::rubble::Reply>>(SendReplyRaw(context));
    }
    std::unique_ptr< ::grpc::ClientAsyncReaderWriterInterface< ::rubble::OpReply, ::rubble::Reply>> AsyncSendReply(::grpc::ClientContext* context, ::grpc::CompletionQueue* cq, void* tag) {
      return std::unique_ptr< ::grpc::ClientAsyncReaderWriterInterface< ::rubble::OpReply, ::rubble::Reply>>(AsyncSendReplyRaw(context, cq, tag));
    }
    std::unique_ptr< ::grpc::ClientAsyncReaderWriterInterface< ::rubble::OpReply, ::rubble::Reply>> PrepareAsyncSendReply(::grpc::ClientContext* context, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncReaderWriterInterface< ::rubble::OpReply, ::rubble::Reply>>(PrepareAsyncSendReplyRaw(context, cq));
    }
    class experimental_async_interface {
     public:
      virtual ~experimental_async_interface() {}
      // an internal rpc call used by the upstream (except the tail node) 
      // to synchronize states to the downstream nodes in the chain
      #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
      virtual void Sync(::grpc::ClientContext* context, ::grpc::ClientBidiReactor< ::rubble::SyncRequest,::rubble::SyncReply>* reactor) = 0;
      #else
      virtual void Sync(::grpc::ClientContext* context, ::grpc::experimental::ClientBidiReactor< ::rubble::SyncRequest,::rubble::SyncReply>* reactor) = 0;
      #endif
      // perform db operations (CRUD)
      #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
      virtual void DoOp(::grpc::ClientContext* context, ::grpc::ClientBidiReactor< ::rubble::Op,::rubble::OpReply>* reactor) = 0;
      #else
      virtual void DoOp(::grpc::ClientContext* context, ::grpc::experimental::ClientBidiReactor< ::rubble::Op,::rubble::OpReply>* reactor) = 0;
      #endif
      // used by the tail node to send the true rely to the replicator
      #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
      virtual void SendReply(::grpc::ClientContext* context, ::grpc::ClientBidiReactor< ::rubble::OpReply,::rubble::Reply>* reactor) = 0;
      #else
      virtual void SendReply(::grpc::ClientContext* context, ::grpc::experimental::ClientBidiReactor< ::rubble::OpReply,::rubble::Reply>* reactor) = 0;
      #endif
    };
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
    typedef class experimental_async_interface async_interface;
    #endif
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
    async_interface* async() { return experimental_async(); }
    #endif
    virtual class experimental_async_interface* experimental_async() { return nullptr; }
  private:
    virtual ::grpc::ClientReaderWriterInterface< ::rubble::SyncRequest, ::rubble::SyncReply>* SyncRaw(::grpc::ClientContext* context) = 0;
    virtual ::grpc::ClientAsyncReaderWriterInterface< ::rubble::SyncRequest, ::rubble::SyncReply>* AsyncSyncRaw(::grpc::ClientContext* context, ::grpc::CompletionQueue* cq, void* tag) = 0;
    virtual ::grpc::ClientAsyncReaderWriterInterface< ::rubble::SyncRequest, ::rubble::SyncReply>* PrepareAsyncSyncRaw(::grpc::ClientContext* context, ::grpc::CompletionQueue* cq) = 0;
    virtual ::grpc::ClientReaderWriterInterface< ::rubble::Op, ::rubble::OpReply>* DoOpRaw(::grpc::ClientContext* context) = 0;
    virtual ::grpc::ClientAsyncReaderWriterInterface< ::rubble::Op, ::rubble::OpReply>* AsyncDoOpRaw(::grpc::ClientContext* context, ::grpc::CompletionQueue* cq, void* tag) = 0;
    virtual ::grpc::ClientAsyncReaderWriterInterface< ::rubble::Op, ::rubble::OpReply>* PrepareAsyncDoOpRaw(::grpc::ClientContext* context, ::grpc::CompletionQueue* cq) = 0;
    virtual ::grpc::ClientReaderWriterInterface< ::rubble::OpReply, ::rubble::Reply>* SendReplyRaw(::grpc::ClientContext* context) = 0;
    virtual ::grpc::ClientAsyncReaderWriterInterface< ::rubble::OpReply, ::rubble::Reply>* AsyncSendReplyRaw(::grpc::ClientContext* context, ::grpc::CompletionQueue* cq, void* tag) = 0;
    virtual ::grpc::ClientAsyncReaderWriterInterface< ::rubble::OpReply, ::rubble::Reply>* PrepareAsyncSendReplyRaw(::grpc::ClientContext* context, ::grpc::CompletionQueue* cq) = 0;
  };
  class Stub final : public StubInterface {
   public:
    Stub(const std::shared_ptr< ::grpc::ChannelInterface>& channel);
    std::unique_ptr< ::grpc::ClientReaderWriter< ::rubble::SyncRequest, ::rubble::SyncReply>> Sync(::grpc::ClientContext* context) {
      return std::unique_ptr< ::grpc::ClientReaderWriter< ::rubble::SyncRequest, ::rubble::SyncReply>>(SyncRaw(context));
    }
    std::unique_ptr<  ::grpc::ClientAsyncReaderWriter< ::rubble::SyncRequest, ::rubble::SyncReply>> AsyncSync(::grpc::ClientContext* context, ::grpc::CompletionQueue* cq, void* tag) {
      return std::unique_ptr< ::grpc::ClientAsyncReaderWriter< ::rubble::SyncRequest, ::rubble::SyncReply>>(AsyncSyncRaw(context, cq, tag));
    }
    std::unique_ptr<  ::grpc::ClientAsyncReaderWriter< ::rubble::SyncRequest, ::rubble::SyncReply>> PrepareAsyncSync(::grpc::ClientContext* context, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncReaderWriter< ::rubble::SyncRequest, ::rubble::SyncReply>>(PrepareAsyncSyncRaw(context, cq));
    }
    std::unique_ptr< ::grpc::ClientReaderWriter< ::rubble::Op, ::rubble::OpReply>> DoOp(::grpc::ClientContext* context) {
      return std::unique_ptr< ::grpc::ClientReaderWriter< ::rubble::Op, ::rubble::OpReply>>(DoOpRaw(context));
    }
    std::unique_ptr<  ::grpc::ClientAsyncReaderWriter< ::rubble::Op, ::rubble::OpReply>> AsyncDoOp(::grpc::ClientContext* context, ::grpc::CompletionQueue* cq, void* tag) {
      return std::unique_ptr< ::grpc::ClientAsyncReaderWriter< ::rubble::Op, ::rubble::OpReply>>(AsyncDoOpRaw(context, cq, tag));
    }
    std::unique_ptr<  ::grpc::ClientAsyncReaderWriter< ::rubble::Op, ::rubble::OpReply>> PrepareAsyncDoOp(::grpc::ClientContext* context, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncReaderWriter< ::rubble::Op, ::rubble::OpReply>>(PrepareAsyncDoOpRaw(context, cq));
    }
    std::unique_ptr< ::grpc::ClientReaderWriter< ::rubble::OpReply, ::rubble::Reply>> SendReply(::grpc::ClientContext* context) {
      return std::unique_ptr< ::grpc::ClientReaderWriter< ::rubble::OpReply, ::rubble::Reply>>(SendReplyRaw(context));
    }
    std::unique_ptr<  ::grpc::ClientAsyncReaderWriter< ::rubble::OpReply, ::rubble::Reply>> AsyncSendReply(::grpc::ClientContext* context, ::grpc::CompletionQueue* cq, void* tag) {
      return std::unique_ptr< ::grpc::ClientAsyncReaderWriter< ::rubble::OpReply, ::rubble::Reply>>(AsyncSendReplyRaw(context, cq, tag));
    }
    std::unique_ptr<  ::grpc::ClientAsyncReaderWriter< ::rubble::OpReply, ::rubble::Reply>> PrepareAsyncSendReply(::grpc::ClientContext* context, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncReaderWriter< ::rubble::OpReply, ::rubble::Reply>>(PrepareAsyncSendReplyRaw(context, cq));
    }
    class experimental_async final :
      public StubInterface::experimental_async_interface {
     public:
      #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
      void Sync(::grpc::ClientContext* context, ::grpc::ClientBidiReactor< ::rubble::SyncRequest,::rubble::SyncReply>* reactor) override;
      #else
      void Sync(::grpc::ClientContext* context, ::grpc::experimental::ClientBidiReactor< ::rubble::SyncRequest,::rubble::SyncReply>* reactor) override;
      #endif
      #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
      void DoOp(::grpc::ClientContext* context, ::grpc::ClientBidiReactor< ::rubble::Op,::rubble::OpReply>* reactor) override;
      #else
      void DoOp(::grpc::ClientContext* context, ::grpc::experimental::ClientBidiReactor< ::rubble::Op,::rubble::OpReply>* reactor) override;
      #endif
      #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
      void SendReply(::grpc::ClientContext* context, ::grpc::ClientBidiReactor< ::rubble::OpReply,::rubble::Reply>* reactor) override;
      #else
      void SendReply(::grpc::ClientContext* context, ::grpc::experimental::ClientBidiReactor< ::rubble::OpReply,::rubble::Reply>* reactor) override;
      #endif
     private:
      friend class Stub;
      explicit experimental_async(Stub* stub): stub_(stub) { }
      Stub* stub() { return stub_; }
      Stub* stub_;
    };
    class experimental_async_interface* experimental_async() override { return &async_stub_; }

   private:
    std::shared_ptr< ::grpc::ChannelInterface> channel_;
    class experimental_async async_stub_{this};
    ::grpc::ClientReaderWriter< ::rubble::SyncRequest, ::rubble::SyncReply>* SyncRaw(::grpc::ClientContext* context) override;
    ::grpc::ClientAsyncReaderWriter< ::rubble::SyncRequest, ::rubble::SyncReply>* AsyncSyncRaw(::grpc::ClientContext* context, ::grpc::CompletionQueue* cq, void* tag) override;
    ::grpc::ClientAsyncReaderWriter< ::rubble::SyncRequest, ::rubble::SyncReply>* PrepareAsyncSyncRaw(::grpc::ClientContext* context, ::grpc::CompletionQueue* cq) override;
    ::grpc::ClientReaderWriter< ::rubble::Op, ::rubble::OpReply>* DoOpRaw(::grpc::ClientContext* context) override;
    ::grpc::ClientAsyncReaderWriter< ::rubble::Op, ::rubble::OpReply>* AsyncDoOpRaw(::grpc::ClientContext* context, ::grpc::CompletionQueue* cq, void* tag) override;
    ::grpc::ClientAsyncReaderWriter< ::rubble::Op, ::rubble::OpReply>* PrepareAsyncDoOpRaw(::grpc::ClientContext* context, ::grpc::CompletionQueue* cq) override;
    ::grpc::ClientReaderWriter< ::rubble::OpReply, ::rubble::Reply>* SendReplyRaw(::grpc::ClientContext* context) override;
    ::grpc::ClientAsyncReaderWriter< ::rubble::OpReply, ::rubble::Reply>* AsyncSendReplyRaw(::grpc::ClientContext* context, ::grpc::CompletionQueue* cq, void* tag) override;
    ::grpc::ClientAsyncReaderWriter< ::rubble::OpReply, ::rubble::Reply>* PrepareAsyncSendReplyRaw(::grpc::ClientContext* context, ::grpc::CompletionQueue* cq) override;
    const ::grpc::internal::RpcMethod rpcmethod_Sync_;
    const ::grpc::internal::RpcMethod rpcmethod_DoOp_;
    const ::grpc::internal::RpcMethod rpcmethod_SendReply_;
  };
  static std::unique_ptr<Stub> NewStub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options = ::grpc::StubOptions());

  class Service : public ::grpc::Service {
   public:
    Service();
    virtual ~Service();
    // an internal rpc call used by the upstream (except the tail node) 
    // to synchronize states to the downstream nodes in the chain
    virtual ::grpc::Status Sync(::grpc::ServerContext* context, ::grpc::ServerReaderWriter< ::rubble::SyncReply, ::rubble::SyncRequest>* stream);
    // perform db operations (CRUD)
    virtual ::grpc::Status DoOp(::grpc::ServerContext* context, ::grpc::ServerReaderWriter< ::rubble::OpReply, ::rubble::Op>* stream);
    // used by the tail node to send the true rely to the replicator
    virtual ::grpc::Status SendReply(::grpc::ServerContext* context, ::grpc::ServerReaderWriter< ::rubble::Reply, ::rubble::OpReply>* stream);
  };
  template <class BaseClass>
  class WithAsyncMethod_Sync : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithAsyncMethod_Sync() {
      ::grpc::Service::MarkMethodAsync(0);
    }
    ~WithAsyncMethod_Sync() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status Sync(::grpc::ServerContext* /*context*/, ::grpc::ServerReaderWriter< ::rubble::SyncReply, ::rubble::SyncRequest>* /*stream*/)  override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestSync(::grpc::ServerContext* context, ::grpc::ServerAsyncReaderWriter< ::rubble::SyncReply, ::rubble::SyncRequest>* stream, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncBidiStreaming(0, context, stream, new_call_cq, notification_cq, tag);
    }
  };
  template <class BaseClass>
  class WithAsyncMethod_DoOp : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithAsyncMethod_DoOp() {
      ::grpc::Service::MarkMethodAsync(1);
    }
    ~WithAsyncMethod_DoOp() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status DoOp(::grpc::ServerContext* /*context*/, ::grpc::ServerReaderWriter< ::rubble::OpReply, ::rubble::Op>* /*stream*/)  override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestDoOp(::grpc::ServerContext* context, ::grpc::ServerAsyncReaderWriter< ::rubble::OpReply, ::rubble::Op>* stream, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncBidiStreaming(1, context, stream, new_call_cq, notification_cq, tag);
    }
  };
  template <class BaseClass>
  class WithAsyncMethod_SendReply : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithAsyncMethod_SendReply() {
      ::grpc::Service::MarkMethodAsync(2);
    }
    ~WithAsyncMethod_SendReply() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status SendReply(::grpc::ServerContext* /*context*/, ::grpc::ServerReaderWriter< ::rubble::Reply, ::rubble::OpReply>* /*stream*/)  override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestSendReply(::grpc::ServerContext* context, ::grpc::ServerAsyncReaderWriter< ::rubble::Reply, ::rubble::OpReply>* stream, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncBidiStreaming(2, context, stream, new_call_cq, notification_cq, tag);
    }
  };
  typedef WithAsyncMethod_Sync<WithAsyncMethod_DoOp<WithAsyncMethod_SendReply<Service > > > AsyncService;
  template <class BaseClass>
  class ExperimentalWithCallbackMethod_Sync : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    ExperimentalWithCallbackMethod_Sync() {
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
      ::grpc::Service::
    #else
      ::grpc::Service::experimental().
    #endif
        MarkMethodCallback(0,
          new ::grpc::internal::CallbackBidiHandler< ::rubble::SyncRequest, ::rubble::SyncReply>(
            [this](
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
                   ::grpc::CallbackServerContext*
    #else
                   ::grpc::experimental::CallbackServerContext*
    #endif
                     context) { return this->Sync(context); }));
    }
    ~ExperimentalWithCallbackMethod_Sync() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status Sync(::grpc::ServerContext* /*context*/, ::grpc::ServerReaderWriter< ::rubble::SyncReply, ::rubble::SyncRequest>* /*stream*/)  override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
    virtual ::grpc::ServerBidiReactor< ::rubble::SyncRequest, ::rubble::SyncReply>* Sync(
      ::grpc::CallbackServerContext* /*context*/)
    #else
    virtual ::grpc::experimental::ServerBidiReactor< ::rubble::SyncRequest, ::rubble::SyncReply>* Sync(
      ::grpc::experimental::CallbackServerContext* /*context*/)
    #endif
      { return nullptr; }
  };
  template <class BaseClass>
  class ExperimentalWithCallbackMethod_DoOp : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    ExperimentalWithCallbackMethod_DoOp() {
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
      ::grpc::Service::
    #else
      ::grpc::Service::experimental().
    #endif
        MarkMethodCallback(1,
          new ::grpc::internal::CallbackBidiHandler< ::rubble::Op, ::rubble::OpReply>(
            [this](
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
                   ::grpc::CallbackServerContext*
    #else
                   ::grpc::experimental::CallbackServerContext*
    #endif
                     context) { return this->DoOp(context); }));
    }
    ~ExperimentalWithCallbackMethod_DoOp() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status DoOp(::grpc::ServerContext* /*context*/, ::grpc::ServerReaderWriter< ::rubble::OpReply, ::rubble::Op>* /*stream*/)  override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
    virtual ::grpc::ServerBidiReactor< ::rubble::Op, ::rubble::OpReply>* DoOp(
      ::grpc::CallbackServerContext* /*context*/)
    #else
    virtual ::grpc::experimental::ServerBidiReactor< ::rubble::Op, ::rubble::OpReply>* DoOp(
      ::grpc::experimental::CallbackServerContext* /*context*/)
    #endif
      { return nullptr; }
  };
  template <class BaseClass>
  class ExperimentalWithCallbackMethod_SendReply : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    ExperimentalWithCallbackMethod_SendReply() {
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
      ::grpc::Service::
    #else
      ::grpc::Service::experimental().
    #endif
        MarkMethodCallback(2,
          new ::grpc::internal::CallbackBidiHandler< ::rubble::OpReply, ::rubble::Reply>(
            [this](
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
                   ::grpc::CallbackServerContext*
    #else
                   ::grpc::experimental::CallbackServerContext*
    #endif
                     context) { return this->SendReply(context); }));
    }
    ~ExperimentalWithCallbackMethod_SendReply() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status SendReply(::grpc::ServerContext* /*context*/, ::grpc::ServerReaderWriter< ::rubble::Reply, ::rubble::OpReply>* /*stream*/)  override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
    virtual ::grpc::ServerBidiReactor< ::rubble::OpReply, ::rubble::Reply>* SendReply(
      ::grpc::CallbackServerContext* /*context*/)
    #else
    virtual ::grpc::experimental::ServerBidiReactor< ::rubble::OpReply, ::rubble::Reply>* SendReply(
      ::grpc::experimental::CallbackServerContext* /*context*/)
    #endif
      { return nullptr; }
  };
  #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
  typedef ExperimentalWithCallbackMethod_Sync<ExperimentalWithCallbackMethod_DoOp<ExperimentalWithCallbackMethod_SendReply<Service > > > CallbackService;
  #endif

  typedef ExperimentalWithCallbackMethod_Sync<ExperimentalWithCallbackMethod_DoOp<ExperimentalWithCallbackMethod_SendReply<Service > > > ExperimentalCallbackService;
  template <class BaseClass>
  class WithGenericMethod_Sync : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithGenericMethod_Sync() {
      ::grpc::Service::MarkMethodGeneric(0);
    }
    ~WithGenericMethod_Sync() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status Sync(::grpc::ServerContext* /*context*/, ::grpc::ServerReaderWriter< ::rubble::SyncReply, ::rubble::SyncRequest>* /*stream*/)  override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
  };
  template <class BaseClass>
  class WithGenericMethod_DoOp : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithGenericMethod_DoOp() {
      ::grpc::Service::MarkMethodGeneric(1);
    }
    ~WithGenericMethod_DoOp() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status DoOp(::grpc::ServerContext* /*context*/, ::grpc::ServerReaderWriter< ::rubble::OpReply, ::rubble::Op>* /*stream*/)  override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
  };
  template <class BaseClass>
  class WithGenericMethod_SendReply : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithGenericMethod_SendReply() {
      ::grpc::Service::MarkMethodGeneric(2);
    }
    ~WithGenericMethod_SendReply() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status SendReply(::grpc::ServerContext* /*context*/, ::grpc::ServerReaderWriter< ::rubble::Reply, ::rubble::OpReply>* /*stream*/)  override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
  };
  template <class BaseClass>
  class WithRawMethod_Sync : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithRawMethod_Sync() {
      ::grpc::Service::MarkMethodRaw(0);
    }
    ~WithRawMethod_Sync() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status Sync(::grpc::ServerContext* /*context*/, ::grpc::ServerReaderWriter< ::rubble::SyncReply, ::rubble::SyncRequest>* /*stream*/)  override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestSync(::grpc::ServerContext* context, ::grpc::ServerAsyncReaderWriter< ::grpc::ByteBuffer, ::grpc::ByteBuffer>* stream, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncBidiStreaming(0, context, stream, new_call_cq, notification_cq, tag);
    }
  };
  template <class BaseClass>
  class WithRawMethod_DoOp : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithRawMethod_DoOp() {
      ::grpc::Service::MarkMethodRaw(1);
    }
    ~WithRawMethod_DoOp() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status DoOp(::grpc::ServerContext* /*context*/, ::grpc::ServerReaderWriter< ::rubble::OpReply, ::rubble::Op>* /*stream*/)  override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestDoOp(::grpc::ServerContext* context, ::grpc::ServerAsyncReaderWriter< ::grpc::ByteBuffer, ::grpc::ByteBuffer>* stream, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncBidiStreaming(1, context, stream, new_call_cq, notification_cq, tag);
    }
  };
  template <class BaseClass>
  class WithRawMethod_SendReply : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithRawMethod_SendReply() {
      ::grpc::Service::MarkMethodRaw(2);
    }
    ~WithRawMethod_SendReply() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status SendReply(::grpc::ServerContext* /*context*/, ::grpc::ServerReaderWriter< ::rubble::Reply, ::rubble::OpReply>* /*stream*/)  override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestSendReply(::grpc::ServerContext* context, ::grpc::ServerAsyncReaderWriter< ::grpc::ByteBuffer, ::grpc::ByteBuffer>* stream, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncBidiStreaming(2, context, stream, new_call_cq, notification_cq, tag);
    }
  };
  template <class BaseClass>
  class ExperimentalWithRawCallbackMethod_Sync : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    ExperimentalWithRawCallbackMethod_Sync() {
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
      ::grpc::Service::
    #else
      ::grpc::Service::experimental().
    #endif
        MarkMethodRawCallback(0,
          new ::grpc::internal::CallbackBidiHandler< ::grpc::ByteBuffer, ::grpc::ByteBuffer>(
            [this](
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
                   ::grpc::CallbackServerContext*
    #else
                   ::grpc::experimental::CallbackServerContext*
    #endif
                     context) { return this->Sync(context); }));
    }
    ~ExperimentalWithRawCallbackMethod_Sync() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status Sync(::grpc::ServerContext* /*context*/, ::grpc::ServerReaderWriter< ::rubble::SyncReply, ::rubble::SyncRequest>* /*stream*/)  override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
    virtual ::grpc::ServerBidiReactor< ::grpc::ByteBuffer, ::grpc::ByteBuffer>* Sync(
      ::grpc::CallbackServerContext* /*context*/)
    #else
    virtual ::grpc::experimental::ServerBidiReactor< ::grpc::ByteBuffer, ::grpc::ByteBuffer>* Sync(
      ::grpc::experimental::CallbackServerContext* /*context*/)
    #endif
      { return nullptr; }
  };
  template <class BaseClass>
  class ExperimentalWithRawCallbackMethod_DoOp : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    ExperimentalWithRawCallbackMethod_DoOp() {
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
      ::grpc::Service::
    #else
      ::grpc::Service::experimental().
    #endif
        MarkMethodRawCallback(1,
          new ::grpc::internal::CallbackBidiHandler< ::grpc::ByteBuffer, ::grpc::ByteBuffer>(
            [this](
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
                   ::grpc::CallbackServerContext*
    #else
                   ::grpc::experimental::CallbackServerContext*
    #endif
                     context) { return this->DoOp(context); }));
    }
    ~ExperimentalWithRawCallbackMethod_DoOp() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status DoOp(::grpc::ServerContext* /*context*/, ::grpc::ServerReaderWriter< ::rubble::OpReply, ::rubble::Op>* /*stream*/)  override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
    virtual ::grpc::ServerBidiReactor< ::grpc::ByteBuffer, ::grpc::ByteBuffer>* DoOp(
      ::grpc::CallbackServerContext* /*context*/)
    #else
    virtual ::grpc::experimental::ServerBidiReactor< ::grpc::ByteBuffer, ::grpc::ByteBuffer>* DoOp(
      ::grpc::experimental::CallbackServerContext* /*context*/)
    #endif
      { return nullptr; }
  };
  template <class BaseClass>
  class ExperimentalWithRawCallbackMethod_SendReply : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    ExperimentalWithRawCallbackMethod_SendReply() {
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
      ::grpc::Service::
    #else
      ::grpc::Service::experimental().
    #endif
        MarkMethodRawCallback(2,
          new ::grpc::internal::CallbackBidiHandler< ::grpc::ByteBuffer, ::grpc::ByteBuffer>(
            [this](
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
                   ::grpc::CallbackServerContext*
    #else
                   ::grpc::experimental::CallbackServerContext*
    #endif
                     context) { return this->SendReply(context); }));
    }
    ~ExperimentalWithRawCallbackMethod_SendReply() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status SendReply(::grpc::ServerContext* /*context*/, ::grpc::ServerReaderWriter< ::rubble::Reply, ::rubble::OpReply>* /*stream*/)  override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
    virtual ::grpc::ServerBidiReactor< ::grpc::ByteBuffer, ::grpc::ByteBuffer>* SendReply(
      ::grpc::CallbackServerContext* /*context*/)
    #else
    virtual ::grpc::experimental::ServerBidiReactor< ::grpc::ByteBuffer, ::grpc::ByteBuffer>* SendReply(
      ::grpc::experimental::CallbackServerContext* /*context*/)
    #endif
      { return nullptr; }
  };
  typedef Service StreamedUnaryService;
  typedef Service SplitStreamedService;
  typedef Service StreamedService;
};

}  // namespace rubble


#endif  // GRPC_rubble_5fkv_5fstore_2eproto__INCLUDED