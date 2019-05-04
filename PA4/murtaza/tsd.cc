/*
 *
 * Copyright 2015, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <ctime>
#include <time.h>
#include <map>
#include <iterator>

#include <google/protobuf/timestamp.pb.h>
#include <google/protobuf/duration.pb.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <memory>
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <google/protobuf/util/time_util.h>
#include <grpc++/grpc++.h>

#include "sns.grpc.pb.h"

using google::protobuf::Timestamp;
using google::protobuf::Duration;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;
using csce438::Message;
using csce438::ListReply;
using csce438::Request;
using csce438::Reply;
using csce438::SNSService;
using csce438::HealthService;
using csce438::HealthCheckRequest;
using csce438::HealthCheckResponse;
using csce438::UpdateRequest;
using csce438::UpdateResponse;
using csce438::SNSRouter;
using csce438::ServerInfoRequest;
using csce438::ServerInfoResponse;

/* Debug Toggles */
#define DBG_CLI 0
#define DBG_HBT 0
#define DBG_RST 0
#define DBG_CLT 0
#define DBG_UDT 1

#define SLP_SLV 4
#define SLP_RTR 1

struct Client {
  std::string username;
  bool connected = true;
  int following_file_size = 0;
  std::vector<Client*> client_followers;
  std::vector<Client*> client_following;
  ServerReaderWriter<Message, Message>* stream = 0;
  bool operator==(const Client& c1) const{
    return (username == c1.username);
  }
};

//Routing server stores its data here
struct Svr {
  std::string myRole; //Server role (routing, master, or slave)
  std::string myIp; //Ip is the same whether it is a master or a slave
  std::string myPort;
  std::string otherPort; //Either master or slave port depending on role
  std::string routingServer;
  std::map<std::string, std::string> masterData; //Holds info on other servers
};

//Only used if you're the routing server
std::unique_ptr<HealthService::Stub> Availablestub_;
std::unique_ptr<HealthService::Stub> MasterOnestub_;
std::unique_ptr<HealthService::Stub> MasterTwostub_;

//Only used if you're the slave server
std::unique_ptr<HealthService::Stub> Masterstub_;

//Only used if you're the master server
std::unique_ptr<HealthService::Stub> Routerstub_;

//Vector that stores every client that has been created
std::vector<Client> client_db;

//Data the server has to store based on its role
Svr server_db;

//Helper function used to find a Client object given its username
int find_user(std::string username){
  int index = 0;
  for(Client c : client_db){
    if(c.username == username)
      return index;
    index++;
  }
  return -1;
}

void write_client_db() {
  std::ofstream out("user_list.txt");

  for(Client c : client_db) {
    out << "STARTCLIENT\n";
    // first line is the username
    out << c.username << "\n";

    // second line is the followers
    for(Client *x : c.client_followers) {
      out << x->username << ",";
    }
    out << "\n";

    // third line is the following
    for (Client *x : c.client_following) {
      out << x->username << ",";
    }

    out << "\nENDCLIENT\n";
  }

  out.close();
}

void read_user_list() {
  std::cout << "reading..." << std::endl;
  std::ifstream pFile("user_list.txt");

  if(pFile.peek() == std::ifstream::traits_type::eof()) {
    return;
  }
  else {
    std::string line;
    int row = 0;
    // first pass checks all clients in local db
    while (std::getline(pFile, line))
    { 
      // client's name
      if(row == 1) {
        int i = find_user(line);
        // add the client to the db
        if(i < 0) {
          Client c;
          c.username = line;
          c.connected = false;
          client_db.push_back(c);
        }
      }
      if(line == "STARTCLIENT") {
        row = 0;
      }
      row++;
    }
    pFile.close();

    // second pass through file updates info
    std::ifstream p2File("user_list.txt");
    row = 0;
    std::string curr_client;
    while (std::getline(p2File, line))
    { 
      // client's name
      if(row == 1) {
        curr_client = line;
      }
      // followers
      if(row == 2) {
        std::vector<std::string> vect;
        std::stringstream ss(line);
        std::string i;

        while (ss >> i)
        {   
            i = i.substr(0, i.size()-1);
            vect.push_back(i);

            if (ss.peek() == ',') {
              ss.ignore();
            }

            i = "";
        }

        for(std::string s : vect) {
          std::cout << "following: " << s << "-" << std::endl;
        }
      }
      // following
      if(row == 3) {

      }
      if(line == "STARTCLIENT") {
        row = 0;
      }
      row++;
    }
    p2File.close();
  }
  return;
}

class HealthServiceImpl final : public HealthService::Service {
  Status Check(ServerContext* context, const HealthCheckRequest* request, HealthCheckResponse* response) override {
    response->set_status(1);
    if(DBG_HBT) {
      std::cout << "Heartbeat response sent" << std::endl;
    }
    return Status::OK;
  }

  Status Update(ServerContext* context, const UpdateRequest* request, UpdateResponse* response) override {
    if(server_db.myRole == "router") {
      // router recieved an update that it needs to send out
      // ClientContext contextOne;
      // ClientContext contextTwo;
      // ClientContext contextThree;

      // UpdateResponse replyOne;
      // UpdateResponse replyTwo;
      // UpdateResponse replyThree;

      // MasterOnestub_->Update(&contextOne, request, &replyOne);
      // MasterTwostub_->Update(&contextTwo, request, &replyTwo);
      // MasterThreestub_->Update(&contextThree, request, &replyThree);
    }
    else if(server_db.myRole == "master") {
      // master recieved and update
      // if(request->command() == "login") {

      // }
      // else if(request->command() == "follow") {
        
      // }
      // else if(request->command() == "unfollow") {
        
      // }
      // else if(request->command() == "post") {
        
      // }
    }
    if(DBG_UDT) {
      std::cout << "Udpate response sent" << std::endl;
    }
    return Status::OK;
  }
};

class SNSRouterImpl final : public SNSRouter::Service {
  Status GetConnectInfo(ServerContext* context, const ServerInfoRequest* request, Reply* reply) override {
    std::string ip = server_db.masterData.find("available")->second;
    if(DBG_CLT) {
      std::cout << "IP sent to client: " << ip << std::endl;
    }
    reply->set_msg(ip);
    return Status::OK;
  }
};

class SNSServiceImpl final : public SNSService::Service {
  
  Status List(ServerContext* context, const Request* request, ListReply* list_reply) override {
    Client user = client_db[find_user(request->username())];
    int index = 0;
    for(Client c : client_db){
      list_reply->add_all_users(c.username);
    }
    std::vector<Client*>::const_iterator it;
    for(it = user.client_followers.begin(); it!=user.client_followers.end(); it++){
      list_reply->add_followers((*it)->username);
    }
    return Status::OK;
  }

  Status Follow(ServerContext* context, const Request* request, Reply* reply) override {
    std::string username1 = request->username();
    std::string username2 = request->arguments(0);
    int join_index = find_user(username2);
    if(join_index < 0 || username1 == username2)
      reply->set_msg("Join Failed -- Invalid Username");
    else{
      Client *user1 = &client_db[find_user(username1)];
      Client *user2 = &client_db[join_index];
      if(std::find(user1->client_following.begin(), user1->client_following.end(), user2) != user1->client_following.end()){
	reply->set_msg("Join Failed -- Already Following User");
        return Status::OK;
      }
      user1->client_following.push_back(user2);
      user2->client_followers.push_back(user1);
      reply->set_msg("Join Successful");
    }
    write_client_db();
    return Status::OK; 
  }

  Status UnFollow(ServerContext* context, const Request* request, Reply* reply) override {
    std::string username1 = request->username();
    std::string username2 = request->arguments(0);
    int leave_index = find_user(username2);
    if(leave_index < 0 || username1 == username2)
      reply->set_msg("Leave Failed -- Invalid Username");
    else{
      Client *user1 = &client_db[find_user(username1)];
      Client *user2 = &client_db[leave_index];
      if(std::find(user1->client_following.begin(), user1->client_following.end(), user2) == user1->client_following.end()){
	reply->set_msg("Leave Failed -- Not Following User");
        return Status::OK;
      }
      user1->client_following.erase(find(user1->client_following.begin(), user1->client_following.end(), user2)); 
      user2->client_followers.erase(find(user2->client_followers.begin(), user2->client_followers.end(), user1));
      reply->set_msg("Leave Successful");
    }
    write_client_db();
    return Status::OK;
  }
  
  Status Login(ServerContext* context, const Request* request, Reply* reply) override {
    Client c;
    std::string username = request->username();
    int user_index = find_user(username);
    if(user_index < 0){
      c.username = username;
      client_db.push_back(c);
      reply->set_msg("Login Successful!");
      
      // update the shared db with the new client
      std::ofstream outfile;
      outfile.open("user_list.txt", std::ios::app);

      outfile << "STARTCLIENT\n";
      // first line is the username
      outfile << c.username << "\n";
      // second line is the followers
      for(Client *x : c.client_followers) {
        outfile << x->username << ",";
      }
      outfile << "\n";
      // third line is the following
      for (Client *x : c.client_following) {
        outfile << x->username << ",";
      }
      outfile << "\nENDCLIENT\n";
      outfile.close();
    }
    else{ 
      Client *user = &client_db[user_index];
      if(user->connected)
        reply->set_msg("Invalid Username");
      else{
        std::string msg = "Welcome Back " + user->username;
	reply->set_msg(msg);
        user->connected = true;
      }
    }
    return Status::OK;
  }

  Status Timeline(ServerContext* context, 
		ServerReaderWriter<Message, Message>* stream) override {
    Message message;
    Client *c;
    while(stream->Read(&message)) {
      std::string username = message.username();
      int user_index = find_user(username);
      c = &client_db[user_index];
 
      //Write the current message to "username.txt"
      std::string filename = username+".txt";
      std::ofstream user_file(filename,std::ios::app|std::ios::out|std::ios::in);
      google::protobuf::Timestamp temptime = message.timestamp();
      std::string time = google::protobuf::util::TimeUtil::ToString(temptime);
      std::string fileinput = time+" :: "+message.username()+":"+message.msg()+"\n";
      //"Set Stream" is the default message from the client to initialize the stream
      if(message.msg() != "Set Stream")
        user_file << fileinput;
      //If message = "Set Stream", print the first 20 chats from the people you follow
      else{
        if(c->stream==0)
      	  c->stream = stream;
        std::string line;
        std::vector<std::string> newest_twenty;
        std::ifstream in(username+"following.txt");
        int count = 0;
        //Read the last up-to-20 lines (newest 20 messages) from userfollowing.txt
        while(getline(in, line)){
          if(c->following_file_size > 20){
	    if(count < c->following_file_size-20){
              count++;
	      continue;
            }
          }
          newest_twenty.push_back(line);
        }
        Message new_msg; 
 	//Send the newest messages to the client to be displayed
	for(int i = 0; i<newest_twenty.size(); i++){
	  new_msg.set_msg(newest_twenty[i]);
          stream->Write(new_msg);
        }    
        continue;
      }
      //Send the message to each follower's stream
      std::vector<Client*>::const_iterator it;
      for(it = c->client_followers.begin(); it!=c->client_followers.end(); it++){
        Client *temp_client = *it;
      	if(temp_client->stream!=0 && temp_client->connected)
	  temp_client->stream->Write(message);
        //For each of the current user's followers, put the message in their following.txt file
        std::string temp_username = temp_client->username;
        std::string temp_file = temp_username + "following.txt";
	std::ofstream following_file(temp_file,std::ios::app|std::ios::out|std::ios::in);
	following_file << fileinput;
        temp_client->following_file_size++;
	std::ofstream user_file(temp_username + ".txt",std::ios::app|std::ios::out|std::ios::in);
        user_file << fileinput;
      }
    }
    //If the client disconnected from Chat Mode, set connected to false
    c->connected = false;
    return Status::OK;
  }

};

//Returns the status of the server
int Check(std::string server) {
    HealthCheckRequest request;
    request.set_service(server_db.myIp);
    HealthCheckResponse reply;
    ClientContext context;

    int s = 0;
    Status status;
    if(server == "available") {
      status = Availablestub_->Check(&context, request, &reply);
      s = reply.status();
    }
    else if(server == "masterOne") {
      status = MasterOnestub_->Check(&context, request, &reply);
      s = reply.status();
    }
    else if(server == "masterTwo") {
      status = MasterTwostub_->Check(&context, request, &reply);
      s = reply.status();
    }
    else if(server == "master") {
      status = Masterstub_->Check(&context, request, &reply);
      s = reply.status();
    }

    if(DBG_HBT) {
      std::cout << server_db.masterData.find(server)->second << " => " << s << std::endl; 
    }

    return s;
}

void Connect_To() {
  //Create channels/stubs
  if(server_db.myRole == "router") {
    Availablestub_ = std::unique_ptr<HealthService::Stub>(HealthService::NewStub(
      grpc::CreateChannel(
        server_db.masterData.find("available")->second, grpc::InsecureChannelCredentials()))); 
    MasterOnestub_ = std::unique_ptr<HealthService::Stub>(HealthService::NewStub(
      grpc::CreateChannel(
        server_db.masterData.find("masterOne")->second, grpc::InsecureChannelCredentials()))); 
    MasterTwostub_ = std::unique_ptr<HealthService::Stub>(HealthService::NewStub(
      grpc::CreateChannel(
        server_db.masterData.find("masterTwo")->second, grpc::InsecureChannelCredentials()))); 
  }
  if(server_db.myRole == "slave") {
    std::string hostname = "localhost:" + server_db.otherPort;
    Masterstub_ = std::unique_ptr<HealthService::Stub>(HealthService::NewStub(
      grpc::CreateChannel(
        hostname, grpc::InsecureChannelCredentials()))); 
  }
  return;
}

void RunServer(std::string port_no) {
  std::string server_address = "0.0.0.0:"+port_no;
  SNSServiceImpl service;
  HealthServiceImpl healthService;
  SNSRouterImpl routerService;

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  builder.RegisterService(&healthService);
  builder.RegisterService(&routerService);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  if(server_db.myRole == "router") {
    while(1) {
      int err = Check("available"); //Only care if available goes down
      if(!err) {
        //make one of the masters the new available server
        srand (time(NULL));
        int randNum = rand()%(2-1 + 1) + 1;
        if(randNum == 1) {
          std::string masterIp = server_db.masterData.find("masterOne")->second;
          std::string availableIp = server_db.masterData.find("available")->second;
          server_db.masterData.find("masterOne")->second = availableIp;
          server_db.masterData.find("available")->second = masterIp;
        } else {
          std::string masterIp = server_db.masterData.find("masterTwo")->second;
          std::string availableIp = server_db.masterData.find("available")->second;
          server_db.masterData.find("masterTwo")->second = availableIp;
          server_db.masterData.find("available")->second = masterIp;
        }

        //Reconnect with updated channel information
        Connect_To();
      }
      sleep(SLP_RTR);
      // err = Check("masterOne");
      // err = Check("masterTwo");
    }
  }
  if(server_db.myRole == "slave") {
    while(1) {
      int err = Check("master");
      if(!err) {
        pid_t pid;
        if((pid = fork()) < 0) {
          //error
        } else if (pid == 0) {
          //We are the child
          char* command = "./tsd";
          char* args[6];
          args[0] = "./tsd";
          std::string arg = "-p " + server_db.otherPort;
          args[1] = (char*)arg.c_str();
          args[2] = "-r master";
          std::string arg2 = "-o " + server_db.myPort;
          args[3] = (char*)arg2.c_str();
          args[4] = "&";
          args[5] = NULL;
          if(execvp(command,args) < 0) {
            //error msg
            //exec failed
            std::cout << "Execvp failed master was not restarted" << std::endl;
            exit(1);
          }
        }
      }
      sleep(SLP_SLV);
    }
  }
  if(server_db.myRole == "master") {
    read_user_list();
  }

  server->Wait();
}

int main(int argc, char** argv) {
  
  std::string port = "3010";
  int opt = 0;
  while ((opt = getopt(argc, argv, "p:r:i:o:a:m:n:s:")) != -1){
    switch(opt) {
      case 'p': // port
          port = optarg;break;
      case 'r': // role
          server_db.myRole = optarg;break;
      case 'i': // ip
          server_db.myIp = optarg;break;
      case 'o': // other (master or slave ip)
          server_db.otherPort = optarg;break;
      case 'a': // available server ip
          server_db.masterData.insert(std::pair<std::string, std::string>("available", optarg));break;
      case 'm': // master server one ip
          server_db.masterData.insert(std::pair<std::string, std::string>("masterOne", optarg));break;
      case 'n': // master server two ip
          server_db.masterData.insert(std::pair<std::string, std::string>("masterTwo", optarg));break;
      case 's':
          server_db.routingServer = optarg;break;
      default:
	  std::cerr << "Invalid Command Line Argument\n";
    }
  }
  server_db.myPort = port;
  Connect_To();

  if(DBG_CLI) {
    std::cout << "Role: " << server_db.myRole << std::endl
    << "Ip: " << server_db.myIp << std::endl
    << "My Port: " << server_db.myPort << std::endl
    << "Slave/Master Port: " << server_db.otherPort << std::endl;

    std::map<std::string, std::string>::iterator itrTest; 
    for (itrTest = server_db.masterData.begin(); itrTest != server_db.masterData.end(); ++itrTest) { 
        std::cout << itrTest->first << ": " << itrTest->second << '\n'; 
    } 
  }

  RunServer(port);

  return 0;
}
