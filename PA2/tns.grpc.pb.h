// Generated by the gRPC C++ plugin.
// If you make any local change, they will be lost.
// source: tns.proto
#ifndef GRPC_tns_2eproto__INCLUDED
#define GRPC_tns_2eproto__INCLUDED

#include "tns.pb.h"

#include <grpc++/impl/codegen/async_stream.h>
#include <grpc++/impl/codegen/async_unary_call.h>
#include <grpc++/impl/codegen/method_handler_impl.h>
#include <grpc++/impl/codegen/proto_utils.h>
#include <grpc++/impl/codegen/rpc_method.h>
#include <grpc++/impl/codegen/service_type.h>
#include <grpc++/impl/codegen/status.h>
#include <grpc++/impl/codegen/stub_options.h>
#include <grpc++/impl/codegen/sync_stream.h>

namespace grpc {
class CompletionQueue;
class Channel;
class ServerCompletionQueue;
class ServerContext;
}  // namespace grpc

namespace tns {

// The greeting service definition.
class Test final {
 public:
  static constexpr char const* service_full_name() {
    return "tns.Test";
  }
  class StubInterface {
   public:
    virtual ~StubInterface() {}
    // Sends multiple greetings
    virtual ::grpc::Status SayHello(::grpc::ClientContext* context, const ::tns::TestRequest& request, ::tns::TestReply* response) = 0;
    std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::tns::TestReply>> AsyncSayHello(::grpc::ClientContext* context, const ::tns::TestRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::tns::TestReply>>(AsyncSayHelloRaw(context, request, cq));
    }
    std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::tns::TestReply>> PrepareAsyncSayHello(::grpc::ClientContext* context, const ::tns::TestRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::tns::TestReply>>(PrepareAsyncSayHelloRaw(context, request, cq));
    }
  private:
    virtual ::grpc::ClientAsyncResponseReaderInterface< ::tns::TestReply>* AsyncSayHelloRaw(::grpc::ClientContext* context, const ::tns::TestRequest& request, ::grpc::CompletionQueue* cq) = 0;
    virtual ::grpc::ClientAsyncResponseReaderInterface< ::tns::TestReply>* PrepareAsyncSayHelloRaw(::grpc::ClientContext* context, const ::tns::TestRequest& request, ::grpc::CompletionQueue* cq) = 0;
  };
  class Stub final : public StubInterface {
   public:
    Stub(const std::shared_ptr< ::grpc::ChannelInterface>& channel);
    ::grpc::Status SayHello(::grpc::ClientContext* context, const ::tns::TestRequest& request, ::tns::TestReply* response) override;
    std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::tns::TestReply>> AsyncSayHello(::grpc::ClientContext* context, const ::tns::TestRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::tns::TestReply>>(AsyncSayHelloRaw(context, request, cq));
    }
    std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::tns::TestReply>> PrepareAsyncSayHello(::grpc::ClientContext* context, const ::tns::TestRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::tns::TestReply>>(PrepareAsyncSayHelloRaw(context, request, cq));
    }

   private:
    std::shared_ptr< ::grpc::ChannelInterface> channel_;
    ::grpc::ClientAsyncResponseReader< ::tns::TestReply>* AsyncSayHelloRaw(::grpc::ClientContext* context, const ::tns::TestRequest& request, ::grpc::CompletionQueue* cq) override;
    ::grpc::ClientAsyncResponseReader< ::tns::TestReply>* PrepareAsyncSayHelloRaw(::grpc::ClientContext* context, const ::tns::TestRequest& request, ::grpc::CompletionQueue* cq) override;
    const ::grpc::internal::RpcMethod rpcmethod_SayHello_;
  };
  static std::unique_ptr<Stub> NewStub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options = ::grpc::StubOptions());

  class Service : public ::grpc::Service {
   public:
    Service();
    virtual ~Service();
    // Sends multiple greetings
    virtual ::grpc::Status SayHello(::grpc::ServerContext* context, const ::tns::TestRequest* request, ::tns::TestReply* response);
  };
  template <class BaseClass>
  class WithAsyncMethod_SayHello : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service *service) {}
   public:
    WithAsyncMethod_SayHello() {
      ::grpc::Service::MarkMethodAsync(0);
    }
    ~WithAsyncMethod_SayHello() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status SayHello(::grpc::ServerContext* context, const ::tns::TestRequest* request, ::tns::TestReply* response) final override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestSayHello(::grpc::ServerContext* context, ::tns::TestRequest* request, ::grpc::ServerAsyncResponseWriter< ::tns::TestReply>* response, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncUnary(0, context, request, response, new_call_cq, notification_cq, tag);
    }
  };
  typedef WithAsyncMethod_SayHello<Service > AsyncService;
  template <class BaseClass>
  class WithGenericMethod_SayHello : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service *service) {}
   public:
    WithGenericMethod_SayHello() {
      ::grpc::Service::MarkMethodGeneric(0);
    }
    ~WithGenericMethod_SayHello() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status SayHello(::grpc::ServerContext* context, const ::tns::TestRequest* request, ::tns::TestReply* response) final override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
  };
  template <class BaseClass>
  class WithStreamedUnaryMethod_SayHello : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service *service) {}
   public:
    WithStreamedUnaryMethod_SayHello() {
      ::grpc::Service::MarkMethodStreamed(0,
        new ::grpc::internal::StreamedUnaryHandler< ::tns::TestRequest, ::tns::TestReply>(std::bind(&WithStreamedUnaryMethod_SayHello<BaseClass>::StreamedSayHello, this, std::placeholders::_1, std::placeholders::_2)));
    }
    ~WithStreamedUnaryMethod_SayHello() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable regular version of this method
    ::grpc::Status SayHello(::grpc::ServerContext* context, const ::tns::TestRequest* request, ::tns::TestReply* response) final override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    // replace default version of method with streamed unary
    virtual ::grpc::Status StreamedSayHello(::grpc::ServerContext* context, ::grpc::ServerUnaryStreamer< ::tns::TestRequest,::tns::TestReply>* server_unary_streamer) = 0;
  };
  typedef WithStreamedUnaryMethod_SayHello<Service > StreamedUnaryService;
  typedef Service SplitStreamedService;
  typedef WithStreamedUnaryMethod_SayHello<Service > StreamedService;
};

}  // namespace tns


#endif  // GRPC_tns_2eproto__INCLUDED
