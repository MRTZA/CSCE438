#include <iostream>
#include <memory>
#include <thread>
#include <vector>
#include <string>
#include <unistd.h>
#include <grpc++/grpc++.h>
#include "client.h"

#include "sns.grpc.pb.h"
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
using csce438::SNSRouter;
using csce438::ServerInfoRequest;
using csce438::ServerInfoResponse;

Message MakeMessage(const std::string& username, const std::string& msg) {
    Message m;
    m.set_username(username);
    m.set_msg(msg);
    google::protobuf::Timestamp* timestamp = new google::protobuf::Timestamp();
    timestamp->set_seconds(time(NULL));
    timestamp->set_nanos(0);
    m.set_allocated_timestamp(timestamp);
    return m;
}

class Client : public IClient
{
    public:
        Client(const std::string& hname,
               const std::string& uname,
               const std::string& p)
            :hostname(hname), username(uname), port(p)
            {}
    protected:
        virtual int connectTo();
        virtual IReply processCommand(std::string& input);
        virtual void processTimeline();
    private:
        std::string hostname;
        std::string username;
        std::string port;
        // You can have an instance of the client stub
        // as a member variable.
        std::unique_ptr<SNSService::Stub> stub_SNSS_;
        std::unique_ptr<SNSRouter::Stub> stub_SNSR_;

        std::string GetConnectInfo();
        IReply Login();
        IReply List();
        IReply Follow(const std::string& username2);
        IReply UnFollow(const std::string& username2);
        void Timeline(const std::string& username);


};

int main(int argc, char** argv) {

    std::string hostname = "localhost";
    std::string username = "default";
    std::string port = "3010";
    int opt = 0;
    while ((opt = getopt(argc, argv, "h:u:p:")) != -1){
        switch(opt) {
            case 'h':
                hostname = optarg;break;
            case 'u':
                username = optarg;break;
            case 'p':
                port = optarg;break;
            default:
                std::cerr << "Invalid Command Line Argument\n";
        }
    }

    Client myc(hostname, username, port);
    // You MUST invoke "run_client" function to start business logic
    myc.run_client();

    return 0;
}

int Client::connectTo()
{

    // Connect to the routing server
    std::string login_info = hostname + ":" + port;

    stub_SNSR_ = std::unique_ptr<SNSRouter::Stub>(SNSRouter::NewStub(
               grpc::CreateChannel(
                    login_info, grpc::InsecureChannelCredentials())));
    
    // Get connection info from the routing server
    std::string availableServerInfo = GetConnectInfo();
    stub_SNSS_ = std::unique_ptr<SNSService::Stub>(SNSService::NewStub(
               grpc::CreateChannel(
                    availableServerInfo, grpc::InsecureChannelCredentials())));
    
    IReply ire = Login();
    if(!ire.grpc_status.ok()) {
        return -1;
    }
    return 1;
}

std::string Client::GetConnectInfo() {
    ServerInfoRequest request;
    request.set_service("Dicks");
    ClientContext context;

    Reply reply;

    Status status = stub_SNSR_->GetConnectInfo(&context, request, &reply);

    std::string r = reply.msg();
    return r;
}

IReply Client::processCommand(std::string& input)
{
	
    IReply ire;
    std::size_t index = input.find_first_of(" ");
    if (index != std::string::npos) {
        std::string cmd = input.substr(0, index);


        /*
        if (input.length() == index + 1) {
            std::cout << "Invalid Input -- No Arguments Given\n";
        }
        */

        std::string argument = input.substr(index+1, (input.length()-index));

        if (cmd == "FOLLOW") {
            return Follow(argument);
        } else if(cmd == "UNFOLLOW") {
            return UnFollow(argument);
        }
    } else {
        if (input == "LIST") {
            return List();
        } else if (input == "TIMELINE") {
            ire.comm_status = SUCCESS;
            return ire;
        }
    }

    ire.comm_status = FAILURE_INVALID;
    return ire;
}

void Client::processTimeline()
{
    Timeline(username);
	// ------------------------------------------------------------
    // In this function, you are supposed to get into timeline mode.
    // You may need to call a service method to communicate with
    // the server. Use getPostMessage/displayPostMessage functions
    // for both getting and displaying messages in timeline mode.
    // You should use them as you did in hw1.
	// ------------------------------------------------------------

    // ------------------------------------------------------------
    // IMPORTANT NOTICE:
    //
    // Once a user enter to timeline mode , there is no way
    // to command mode. You don't have to worry about this situation,
    // and you can terminate the client program by pressing
    // CTRL-C (SIGINT)
	// ------------------------------------------------------------
}

IReply Client::List() {
    //Data being sent to the server
    Request request;
    request.set_username(username);

    //Container for the data from the server
    ListReply list_reply;

    //Context for the client
    ClientContext context;

    Status status = stub_SNSS_->List(&context, request, &list_reply);
    IReply ire;
    ire.grpc_status = status;
    //Loop through list_reply.all_users and list_reply.following_users
    //Print out the name of each room
    if(status.ok()){
        ire.comm_status = SUCCESS;
        std::string all_users;
        std::string following_users;
        for(std::string s : list_reply.all_users()){
            ire.all_users.push_back(s);
        }
        for(std::string s : list_reply.followers()){
            ire.followers.push_back(s);
        }
    } else {
        std::cout << "Connection failed, reconnecting..." << std::endl;
    }
    return ire;
}
        
IReply Client::Follow(const std::string& username2) {
    Request request;
    request.set_username(username);
    request.add_arguments(username2);

    Reply reply;
    ClientContext context;

    Status status = stub_SNSS_->Follow(&context, request, &reply);
    if(!status.ok()) {
        std::cout << "Connection failed, reconnecting..." << std::endl;
    }
    IReply ire; ire.grpc_status = status;
    if (reply.msg() == "unkown user name") {
        ire.comm_status = FAILURE_INVALID_USERNAME;
    } else if (reply.msg() == "unknown follower username") {
        ire.comm_status = FAILURE_INVALID_USERNAME;
    } else if (reply.msg() == "you have already joined") {
        ire.comm_status = FAILURE_ALREADY_EXISTS;
    } else if (reply.msg() == "Follow Successful") {
        ire.comm_status = SUCCESS;
    } else {
        ire.comm_status = FAILURE_UNKNOWN;
    }
    return ire;
}

IReply Client::UnFollow(const std::string& username2) {
    Request request;

    request.set_username(username);
    request.add_arguments(username2);

    Reply reply;

    ClientContext context;

    Status status = stub_SNSS_->UnFollow(&context, request, &reply);
    if(!status.ok()) {
        std::cout << "Connection failed, reconnecting..." << std::endl;
    }
    IReply ire;
    ire.grpc_status = status;
    if (reply.msg() == "unknown follower username") {
        ire.comm_status = FAILURE_INVALID_USERNAME;
    } else if (reply.msg() == "you are not follower") {
        ire.comm_status = FAILURE_INVALID_USERNAME;
    } else if (reply.msg() == "UnFollow Successful") {
        ire.comm_status = SUCCESS;
    } else {
        ire.comm_status = FAILURE_UNKNOWN;
    }

    return ire;
}

IReply Client::Login() {
    Request request;
    request.set_username(username);
    Reply reply;
    ClientContext context;

    Status status = stub_SNSS_->Login(&context, request, &reply);
    if(!status.ok()) {
        std::cout << "Connection failed, reconnecting..." << std::endl;
    }
    IReply ire;
    ire.grpc_status = status;
    if (reply.msg() == "you have already joined") {
        ire.comm_status = FAILURE_ALREADY_EXISTS;
    } else {
        ire.comm_status = SUCCESS;
    }
    return ire;
}

void Client::Timeline(const std::string& username) {
    ClientContext context;

    std::shared_ptr<ClientReaderWriter<Message, Message>> stream(
            stub_SNSS_->Timeline(&context));

    //Thread used to read chat messages and send them to the server
    std::thread writer([username, stream]() {
            std::string input = "Set Stream";
            Message m = MakeMessage(username, input);
            stream->Write(m);
            while (1) {
            input = getPostMessage();
            m = MakeMessage(username, input);
            stream->Write(m);
            }
            stream->WritesDone();
            });

    std::thread reader([username, stream]() {
            Message m;
            while(stream->Read(&m)){

            google::protobuf::Timestamp temptime = m.timestamp();
            std::time_t time = temptime.seconds();
            displayPostMessage(m.username(), m.msg(), time);
            }
            });

    //Wait for the threads to finish
    writer.join();
    reader.join();
}

