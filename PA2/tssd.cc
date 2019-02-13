#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <grpc++/grpc++.h>

#include "tns.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using tns::TestRequest;
using tns::TestReply;
using tns::Test;

using tns::tinyNetworkingService;

using tns::ListRequest;
using tns::ListReply;

using tns::FollowRequest;
using tns::FollowReply;

/*
* struct to hold user data
*/
struct user {
  std::string name;
  std::vector<user *> following;
  std::vector<user *> followers;
  std::vector<std::string> timeline;
};

/* global variable of all users */
std::vector<user *> users;

// Logic and data behind the server's behavior.
class TestServiceImpl final : public Test::Service {
  Status SayHello(ServerContext* context, const TestRequest* request,
                  TestReply* reply) {
    std::string prefix("Hello ");

    /* update server info on users */
    user *temp = new user();
    temp->name = request->name();
    users.push_back(temp);

    reply->set_message(prefix + request->name());
    return Status::OK;
  }
};

// define server behavior for LIST command
class tnsServiceImpl final : public tinyNetworkingService::Service {
  Status List(ServerContext* context, const ListRequest* request,
                  ListReply* reply) {

    /* get server info on users */
    std::string allUsers = "";
    std::string followingUsers = request->name() + ",";
    for(auto u : users) {
      allUsers += u->name + ",";
      if(u->name == request->name()) {
        for(auto f : u->following) {
          followingUsers += f->name + ",";
        }
      }
    }

    reply->set_all(allUsers);
    reply->set_following(followingUsers);
    reply->set_status(tns::ListReply_IStatus_SUCCESS);
    return Status::OK;
  }
};

void RunServer() {
  std::string server_address("localhost:50051");
  TestServiceImpl service;
  tnsServiceImpl tns;

  ServerBuilder builder;

  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());

  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  builder.RegisterService(&tns);

  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

int main(int argc, char** argv) {
  RunServer();

  return 0;
}