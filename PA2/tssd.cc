#include <iostream>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>
#include <mutex>
#include <ctime>
#include <condition_variable>

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

using tns::UpdateRequest;
using tns::UpdateReply;

using tns::PostRequest;
using tns::PostReply;

using std::cout;
using std::endl;

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
pthread_mutex_t m;

void restoreFromFile() {
  std::ifstream file("server.txt");
  if(file.is_open()) {
    std::string line;
    // Ignore the first line because it is a timestamp
    file.ignore(10000,'\n');
    
    // Read through the file once to get all the users created.
    while(std::getline(file, line)) {
      
      // Add the users we read from the file
      user *newUser = new user();
      newUser->name = line;
      users.push_back(newUser);
      
      // Ignore the user information
      file.ignore(10000,'\n');
      file.ignore(10000,'\n');
      file.ignore(10000,'\n');
    }

    // for(auto u : users)
    //   cout << u->name << endl;


    // Move back to the start of the file to 
    file.clear();
    file.seekg(0, std::ios::beg);

    // Ignore the first line again
    file.ignore(10000,'\n');
    while(std::getline(file, line)) {
      // Find pointer of our current user
      std::string userString = line;
      user *currentUser;
      for(auto u : users)
        if(u->name == userString)
          currentUser = u;

      std::getline(file, line);
      std::stringstream following(line);
      // Add the user who currentUser is following to their following vector
      while(following.good()) {
        std::string userToAdd;
        getline(following,userToAdd,',');
        if(userToAdd != "") {
          for(auto u : users)
            if(u->name == userToAdd)
              currentUser->following.push_back(u);
        }
      }

      std::getline(file, line);
      std::stringstream followers(line);
      // Add the users who follow current user to the followers vector
      while(followers.good()) {
        std::string userToAdd;
        getline(followers,userToAdd,',');
        if(userToAdd != "") {
          for(auto u : users)
            if(u->name == userToAdd)
              currentUser->followers.push_back(u);
        }
      }

      std::getline(file, line);
      std::stringstream timeline(line);
      // Add all the posts to the current users timeline
      while(timeline.good()) {
        std::string postToAdd;
        getline(timeline,postToAdd,',');
        if(postToAdd != "") {
          currentUser->timeline.push_back(postToAdd);
        }
      }
    }
  }
}

void logServerState() {
  //Delete the current server file so we can write our new one
  remove("./server.txt");
  std::ofstream file("server.txt", std::ios::out);
  
  // Get the current time into a string
  time_t rawtime;
  char buffer[80];
  struct tm * timeinfo;
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  strftime(buffer, sizeof(buffer), "%d-%m-%Y %H:%M:%S", timeinfo);
  std::string time(buffer);
  
  // Output all the server info to the file.
  if(file.is_open()) {
    file << time << endl;
    for(auto u : users) {
      //Put users name into file
      file << u->name << endl;
      //Put who user is following into file
      for(auto flg : u->following) 
        file << flg->name << ",";
      file << endl;
      //Put who is following user into file
      for(auto flr : u->followers) 
        file << flr->name << ",";
      file << endl;
      //Put users timeline into file.
      for(auto post : u->timeline)
        file << post << ",";
      file << endl;
    }
    file << "\0";
    file.close();
  }
}

void killServer(int signum) {
  cout << endl << "Closing Server Saving Information" << endl;
  logServerState();
  exit(signum);
}


// Logic and data behind the server's behavior.
class TestServiceImpl final : public Test::Service {
  Status SayHello(ServerContext* context, const TestRequest* request,
                  TestReply* reply) {
    std::string prefix("Hello ");

    //pthread_mutex_lock(&m);

    // if user exists dont add them
    for(auto u : users) {
      if(u->name == request->name()) {
        //pthread_mutex_unlock(&m);
        reply->set_message(prefix + request->name());
        return Status::OK;
      }
    }

    /* update server info on users */
    user *temp = new user();
    temp->name = request->name();
    users.push_back(temp);

    reply->set_message(prefix + request->name());

    //pthread_mutex_unlock(&m);
    return Status::OK;
  }
};

// define server behavior for LIST command
class tnsServiceImpl final : public tinyNetworkingService::Service {
  Status List(ServerContext* context, const ListRequest* request,
                  ListReply* reply) {

    //pthread_mutex_lock(&m);

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

    //pthread_mutex_unlock(&m);
    return Status::OK;
  }

  Status Follow(ServerContext* context, const FollowRequest* request,
                  FollowReply* reply) {
    
    //pthread_mutex_lock(&m);

    bool success = false;
    /* get server info on users */
    for(auto u : users) {
      if(u->name == request->user()) { // Found the user who sent request
        for(auto f : users) { // find the user who we are trying to follow
          if(f->name == request->name()) {
            u->following.push_back(f);
            f->followers.push_back(u);
            success = true;
          }
        }
      }
    }
    
    //pthread_mutex_unlock(&m);

    if(success)
      reply->set_status(tns::FollowReply_IStatus_SUCCESS);
    else
      reply->set_status(tns::FollowReply_IStatus_FAILURE_NOT_EXISTS);
    return Status::OK;
  }

  Status Unfollow(ServerContext* context, const FollowRequest* request,
                  FollowReply* reply) {
    bool success = false;
    // User who is unfollowing and who is being unfollowed.
    user* us;
    user* fs;

    //pthread_mutex_lock(&m);
    /* get server info on users */
    for(auto u : users) {
      if(u->name == request->user()) { // Found the user who sent request
        for(auto f : u->following) { // find the user who we are trying to follow
          if(f->name == request->name()) {
            us = u;
            fs = f;
            success = true;
          }
        }
      }
    }

    //pthread_mutex_unlock(&m);

    if(success)
      reply->set_status(tns::FollowReply_IStatus_SUCCESS);
    else {
      reply->set_status(tns::FollowReply_IStatus_FAILURE_NOT_EXISTS);
      return Status::OK;
    }
    
    //pthread_mutex_lock(&m);

    for(int i = 0; i < us->following.size(); i++) {
      if(us->following.at(i) == fs) {
        us->following.erase(us->following.begin() + i);
        break;
      }
    }

    for(int i = 0; i < fs->followers.size(); i++) {
      if(fs->followers.at(i) == us) {
        fs->followers.erase(fs->followers.begin() + i);
        break;
      }
    }

    //pthread_mutex_unlock(&m);
    return Status::OK;
  }

  Status Update(ServerContext* context, const UpdateRequest* request,
                  UpdateReply* reply) {
    
    // std::cout << request->name() << " is requesting update, they see " << request->posts() << " posts" << std::endl;
    //pthread_mutex_lock(&m);

    std::string replyString = "";
    std::vector<std::string> timeline;
    /* get server info on users */
    for(auto u : users) {
      if(u->name == request->name()) { // Found the user who sent request
        timeline = u->timeline;
      }
    }

    if(timeline.size() == request->posts()) {
      // std::cout << request->name() << "'s timeline is up to date" << std::endl;
      // they dont need any more updates
      reply->set_status(tns::UpdateReply_IStatus_SUCCESS);
      reply->set_timeline(replyString);
      //pthread_mutex_unlock(&m);
      // std::cout << request->name() << " is finished requesting update" << std::endl;
      return Status::OK; 
    }
    
    //pthread_mutex_lock(&m);

    // they need a certain number of updates
    for(int i = timeline.size() - request->posts()-1; i < timeline.size(); i++) {
      if(i == timeline.size() -1) {
        // check if its the user's own post
        std::string sender = "";
        for(int j = 0; j < timeline[i].length(); j++) {
          if(timeline[i] == "(") {
            break;
          }
          sender += timeline[i];
        }

        if(sender != request->name()) {
          replyString += timeline[i];
        }
      }
      else {
        // check if its the user's own post
        std::string sender = "";
        for(int j = 0; j < timeline[i].length(); j++) {
          if(timeline[i] == "(") {
            break;
          }
          sender += timeline[i];
        }

        if(sender != request->name()) {
          replyString += timeline[i] + ",";
        }
      }
    }

    reply->set_status(tns::UpdateReply_IStatus_SUCCESS);
    reply->set_timeline(replyString);
    //pthread_mutex_unlock(&m);
    // std::cout << request->name() << " is finished requesting update" << std::endl;
    return Status::OK;
  }

  Status Post(ServerContext* context, const PostRequest* request,
                  PostReply* reply) {

    std::cout << request->name() << " submitted a post" << std::endl;

    // build the post
    std::string post = "";
    time_t rawTime;
    char buffer[80];
    struct tm * timeinfo;
    time(&rawTime);
    timeinfo = localtime(&rawTime);
    strftime(buffer, sizeof(buffer), "%d-%m-%Y %H:%M:%S", timeinfo);
    std::string t_str(buffer);

    post = request->name() + "(" + t_str + ") >> " + request->post();

    //pthread_mutex_lock(&m);
    // update the users
    for(auto u : users) {
      if(u->name == request->name()) {
        u->timeline.push_back(post);
        for(auto f : u->followers) {
          std::cout << "adding post to " << request->name() << "'s followers' timeline" << std::endl;
          f->timeline.push_back(post);
        }
      }
    }

    reply->set_status(tns::PostReply_IStatus_SUCCESS);
    //pthread_mutex_unlock(&m);
    std::cout << "finished submitting " << request->name() << "'s post" << std::endl;
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
  pthread_mutex_init(&m, nullptr);
  signal(SIGINT, killServer);
  restoreFromFile();
  RunServer();

  return 0;
}