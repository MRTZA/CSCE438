//#include <iostream>
//#include <memory>
//#include <thread>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <vector>
#include <string>
#include <unistd.h>
#include <pthread.h>
#include <mutex>
#include <grpc++/grpc++.h>
#include "client.h"

#include "tns.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
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

pthread_mutex_t m;

/* Global variable: 
* to keep track of the posts printed for the client
* helps to check for updates against the server
*/
int postSeen = 0;

int readPostSeen() {
    // pthread_mutex_lock(&m);

    return postSeen;

    // pthread_mutex_unlock(&m);
}

void setPostSeen() {
    // pthread_mutex_lock(&m);

    postSeen++;

    // pthread_mutex_unlock(&m);
}   


class Client : public IClient
{
    public:
        Client(const std::string& hname,
               const std::string& uname,
               const std::string& p)
            :hostname(hname), username(uname), port(p){}

        std::string getUsername() { return username; }

        // Assembles the client's payload, sends it and presents the response back
        // from the server.
        std::string SayHello(const std::string& user) {
            // Data we are sending to the server.
            TestRequest request;
            request.set_name(user);

            // Container for the data we expect from the server.
            TestReply reply;

            // Context for the client. It could be used to convey extra information to
            // the server and/or tweak certain RPC behaviors.
            ClientContext context;

            // The actual RPC.
            Status status = stub_->SayHello(&context, request, &reply);

            // Act upon its status.
            if (status.ok()) {
            return reply.message();
            } else {
            std::cout << status.error_code() << ": " << status.error_message()
                        << std::endl;
            return "RPC failed";
            }
        }

        std::vector<std::string> List(const std::string& user, IReply* ireply) {
            // Data we are sending to the server.
            ListRequest request;
            request.set_name(user);

            // Container for the data we expect from the server.
            ListReply reply;

            // Context for the client. It could be used to convey extra information to
            // the server and/or tweak certain RPC behaviors.
            ClientContext context;

            // The actual RPC.
            Status status = tns_stub_->List(&context, request, &reply);

            std::vector<std::string> returnVec;
            // Act upon its status.
            if (status.ok()) {
                returnVec.push_back(reply.all());
                returnVec.push_back(reply.following());
                ireply->comm_status = (IStatus)reply.status();
                return returnVec;
            } else {
            std::cout << status.error_code() << ": " << status.error_message()
                        << std::endl;
                        returnVec.push_back("RPC failed");
            return returnVec;
            }
        }

        IStatus Follow(const std::string& user, const std::string& name) {
            // Data we are sending to the server.
            FollowRequest request;
            request.set_user(user);
            request.set_name(name);

            // Container for the data we expect from the server.
            FollowReply reply;

            // Context for the client. It could be used to convey extra information to
            // the server and/or tweak certain RPC behaviors.
            ClientContext context;

            // The actual RPC.
            Status status = tns_stub_->Follow(&context, request, &reply);

            // Act upon its status.
            if (status.ok()) {
                return (IStatus)reply.status();
            } else {
            std::cout << status.error_code() << ": " << status.error_message()
                        << std::endl;
            return FAILURE_UNKNOWN;
            }
        }

        IStatus Unfollow(const std::string& user, const std::string& name) {
            // Data we are sending to the server.
            FollowRequest request;
            request.set_user(user);
            request.set_name(name);

            // Container for the data we expect from the server.
            FollowReply reply;

            // Context for the client. It could be used to convey extra information to
            // the server and/or tweak certain RPC behaviors.
            ClientContext context;

            // The actual RPC.
            Status status = tns_stub_->Unfollow(&context, request, &reply);

            // Act upon its status.
            if (status.ok()) {
                return (IStatus)reply.status();
            } else {
            std::cout << status.error_code() << ": " << status.error_message()
                        << std::endl;
            return FAILURE_UNKNOWN;
            }
        }

        std::vector<std::string> Update(const std::string& name, int numPosts, IReply* ireply) {
            // Data we are sending to the server.
            UpdateRequest request;
            request.set_name(name);
            request.set_posts(numPosts);

            // Container for the data we expect from the server.
            UpdateReply reply;

            // Context for the client. It could be used to convey extra information to
            // the server and/or tweak certain RPC behaviors.
            ClientContext context;

            // The actual RPC.
            // std::cout << "atempting update" << std::endl;
            Status status = tns_stub_->Update(&context, request, &reply);
            // std::cout << "finished update" << std::endl;

            std::vector<std::string> returnVec;

            // Act upon its status.
            if (status.ok()) {
                ireply->comm_status = (IStatus)reply.status();
                
                // parse the reply string of posts
                if(reply.timeline() == "") {
                    return returnVec;
                }
                if(reply.timeline().find(",") == std::string::npos) {
                    returnVec.push_back(reply.timeline());
                    return returnVec;
                }
            
                std::stringstream ss(reply.timeline());
                while(ss.good()) {
                    std::string substr;
                    getline(ss,substr,',');
                    returnVec.push_back(substr);
                }
                return returnVec;

            } else {
            std::cout << status.error_code() << ": " << status.error_message()
                        << std::endl;
                        returnVec.push_back("RPC failed");
            return returnVec;
            }
        }

        IStatus Post(const std::string& name, std::string message) {
            // Data we are sending to the server.
            PostRequest request;
            request.set_name(name);
            request.set_post(message);

            // Container for the data we expect from the server.
            PostReply reply;

            // Context for the client. It could be used to convey extra information to
            // the server and/or tweak certain RPC behaviors.
            ClientContext context;

            // The actual RPC.
            Status status = tns_stub_->Post(&context, request, &reply);

            // Act upon its status.
            if (status.ok()) {
                return (IStatus)reply.status();
            } else {
            std::cout << status.error_code() << ": " << status.error_message()
                        << std::endl;
            return FAILURE_UNKNOWN;
            }
        }

        void setChannel(std::shared_ptr<Channel> channel) {
            stub_ = Test::NewStub(channel);
            tns_stub_ = tinyNetworkingService::NewStub(channel);
        }

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
        //std::unique_ptr<NameOfYourStubClass::Stub> stub_;
        std::unique_ptr<Test::Stub> stub_;
        std::unique_ptr<tinyNetworkingService::Stub> tns_stub_;
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
	// ------------------------------------------------------------
    // In this function, you are supposed to create a stub so that
    // you call service methods in the processCommand/porcessTimeline
    // functions. That is, the stub should be accessible when you want
    // to call any service methods in those functions.
    // I recommend you to have the stub as
    // a member variable in your own Client class.
    // Please refer to gRpc tutorial how to create a stub.
	// ------------------------------------------------------------

    // Instantiate the client. It requires a channel, out of which the actual RPCs
    // are created. This channel models a connection to an endpoint (in this case,
    // localhost at port 50051). We indicate that the channel isn't authenticated
    // (use of InsecureChannelCredentials()).
    std::string server_address = hostname + ":" + port;
    setChannel(grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials()));

    std::string reply = SayHello(username);

    if(reply == "RPC failed") 
        return -1; // return 1 if success, otherwise return -1
    else 
        return 1;
}

IReply Client::processCommand(std::string& input)
{
	// ------------------------------------------------------------
	// GUIDE 1:
	// In this function, you are supposed to parse the given input
    // command and create your own message so that you call an 
    // appropriate service method. The input command will be one
    // of the followings:
	//
	// FOLLOW <username>
	// UNFOLLOW <username>
	// LIST
    // TIMELINE
	//
	// - JOIN/LEAVE and "<username>" are separated by one space.
	// ------------------------------------------------------------

    // ------------------------------------------------------------
	// GUIDE 2:
	// Then, you should create a variable of IReply structure
	// provided by the client.h and initialize it according to
	// the result. Finally you can finish this function by returning
    // the IReply.
	// ------------------------------------------------------------
    
	// ------------------------------------------------------------
    // HINT: How to set the IReply?
    // Suppose you have "Join" service method for JOIN command,
    // IReply can be set as follow:
    // 
    //     // some codes for creating/initializing parameters for
    //     // service method
    //     IReply ire;
    //     grpc::Status status = stub_->Join(&context, /* some parameters */);
    //     ire.grpc_status = status;
    //     if (status.ok()) {
    //         ire.comm_status = SUCCESS;
    //     } else {
    //         ire.comm_status = FAILURE_NOT_EXISTS;
    //     }
    //      
    //      return ire;
    // 
    // IMPORTANT: 
    // For the command "LIST", you should set both "all_users" and 
    // "following_users" member variable of IReply.
	// ------------------------------------------------------------
    
    IReply ire;
    //if(input != "LIST" || input.substr(0,6) != "FOLLOW ") {
    //    ire.comm_status = FAILURE_INVALID;
    //} else 
    if(input == "LIST") {
        std::vector<std::string> result = List(username, &ire);
        std::vector<std::string> temp;
        temp.push_back(result[0]);
        ire.all_users = temp;
        temp[0] = result[1];
        ire.following_users = temp;
    } else if(input.substr(0,7) == "FOLLOW ") {
        ire.comm_status = Follow(username, input.substr(7,input.length()));
    } else if(input.substr(0,9) == "UNFOLLOW ") {
        ire.comm_status = Unfollow(username, input.substr(9,input.length()));
    } else if(input.substr(0,8) == "TIMELINE") {
        std::vector<std::string> timeline = Update(username, postSeen, &ire);
    }
    else {
        ire.comm_status = FAILURE_INVALID;
    }
    
    return ire;
}


void* updateThreadFunction(void* update) {
    Client *updateData = static_cast<Client*>(update);
	for(;;) {
        IReply ire;
        std::vector<std::string> posts = updateData->Update(updateData->getUsername(), readPostSeen(), &ire);
        
        // std::cout << "recieved update of " << posts.size() << " posts" << std::endl;
        
        int num = std::max(posts.size()-20, 0);
        for(int i = posts.size(); i > num; i--) { 
            displayPostMessage(posts[i]);
        }

        for(int i = 0; i < posts.size(); i++) {
            setPostSeen();
        }

        sleep(1);
    }
}

 
void* postThreadFunction(void* post) {
    Client *postData = static_cast<Client*>(post);
	for(;;) {
        std::string message = getPostMessage();

        postData->Post(postData->getUsername(), message);
        setPostSeen();
    }
}

void Client::processTimeline()
{
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
    pthread_t updateThread;
	pthread_t postThread;

    pthread_create(&updateThread, NULL, updateThreadFunction, (void*)this);
	pthread_create(&postThread, NULL, postThreadFunction, (void*)this);

    while(1) {}
}
