#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>

#include <string>
#include <iostream>
#include <bits/stdc++.h> 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "interface.h"

#define BUFFER_LENGTH    250
#define DEBUG			 1

using std::cout;
using std::endl;

/*
 * TODO: IMPLEMENT BELOW THREE FUNCTIONS
 */
int connect_to(const char *host, const int port);
struct Reply process_command(const int sockfd, char* command);
void process_chatmode(const char* host, const int port);
bool process_chatmode_run = true;

int main(int argc, char** argv) 
{
	if (argc != 3) {
		fprintf(stderr,
				"usage: enter host address and port number\n");
		exit(1);
	}

    display_title();
    
	while (1) {
		

		int sockfd = connect_to(argv[1], atoi(argv[2]));
    
		char command[MAX_DATA];
        get_command(command, MAX_DATA);

		struct Reply reply = process_command(sockfd, command);
		display_reply(command, reply);
		
		touppercase(command, strlen(command) - 1);

		if (strncmp(command, "JOIN ", 5) == 0) {
			if(reply.status == SUCCESS) {
				printf("Now you are in the chatmode\n");
				process_chatmode(argv[1], reply.port);
			}
		}
	
		close(sockfd);

    }

    return 0;
}

/*
 * Connect to the server using given host and port information
 *
 * @parameter host    host address given by command line argument
 * @parameter port    port given by command line argument
 * 
 * @return socket fildescriptor
 */
int connect_to(const char *host, const int port)
{
	// ------------------------------------------------------------
	// GUIDE :
	// In this function, you are suppose to connect to the server.
	// After connection is established, you are ready to send or
	// receive the message to/from the server.
	// 
	// Finally, you should return the socket fildescriptor
	// so that other functions such as "process_command" can use it
	// ------------------------------------------------------------

    // Create the socket and store the file descriptor
	int sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd < 0) 
		perror("ERROR opening socket");
	
	// Populate the server connection data
	struct sockaddr_in serveraddr;

	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family      = AF_INET;
	serveraddr.sin_port        = htons(port);
	serveraddr.sin_addr.s_addr = inet_addr(host);

	// Connect to the server
	int err = connect(sockfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr));
	if(err ==0) { 
		printf("Connection Successfull\n");
	}

	return sockfd;
}

/* 
 * Send an input command to the server and return the result
 *
 * @parameter sockfd   socket file descriptor to commnunicate
 *                     with the server
 * @parameter command  command will be sent to the server
 *
 * @return    Reply    
 */
struct Reply process_command(const int sockfd, char* command)
{
	// ------------------------------------------------------------
	// GUIDE 1:
	// In this function, you are supposed to parse a given command
	// and create your own message in order to communicate with
	// the server. Surely, you can use the input command without
	// any changes if your server understand it. The given command
    // will be one of the followings:
	//
	// CREATE <name>
	// DELETE <name>
	// JOIN <name>
    // LIST
	//
	// -  "<name>" is a chatroom name that you want to create, delete,
	// or join.
	// 
	// - CREATE/DELETE/JOIN and "<name>" are separated by one space.
	// ------------------------------------------------------------
	
	// Type cast the command info properly for the send() function
	char buffer[BUFFER_LENGTH];
	strcpy(buffer, command);

	// ------------------------------------------------------------
	// GUIDE 2:
	// After you create the message, you need to send it to the
	// server and receive a result from the server.
	// ------------------------------------------------------------
	int rc = send(sockfd, buffer, sizeof(buffer), 0);
	// test error rc < 0
	if(rc < 0) { perror("Send Unsucessfull"); }

	// ------------------------------------------------------------
	// GUIDE 3:
	// Then, you should create a variable of Reply structure
	// provided by the interface and initialize it according to
	// the result.
	//
	// For example, if a given command is "JOIN room1"
	// and the server successfully created the chatroom,
	// the server will reply a message including information about
	// success/failure, the number of members and port number.
	// By using this information, you should set the Reply variable.
	// the variable will be set as following:
	//
	// Reply reply;
	// reply.status = SUCCESS;
	// reply.num_member = number;
	// reply.port = port;
	// 
	// "number" and "port" variables are just an integer variable
	// and can be initialized using the message fomr the server.
	//
	// For another example, if a given command is "CREATE room1"
	// and the server failed to create the chatroom becuase it
	// already exists, the Reply varible will be set as following:
	//
	// Reply reply;
	// reply.status = FAILURE_ALREADY_EXISTS;
    // 
    // For the "LIST" command,
    // You are suppose to copy the list of chatroom to the list_room
    // variable. Each room name should be seperated by comma ','.
    // For example, if given command is "LIST", the Reply variable
    // will be set as following.
    //
    // Reply reply;
    // reply.status = SUCCESS;
    // strcpy(reply.list_room, list);
    // 
    // "list" is a string that contains a list of chat rooms such 
    // as "r1,r2,r3,"
	// ------------------------------------------------------------

	// REMOVE below code and write your own Reply.

	// Reply struct that needs to be populated
	struct Reply reply;

	// JOIN
	if(strncmp(buffer, "JOIN ", 5) == 0) {
		char charStatus[BUFFER_LENGTH];
		rc = recv(sockfd, charStatus, sizeof(charStatus), 0);
		std::string replyInfo(charStatus);
		
		std::vector <std::string> tokens; 
      
		// stringstream class check1 
		std::stringstream check1(replyInfo); 
		std::string intermediate;  
		// Tokenizing w.r.t. space ' ' 
		while(getline(check1, intermediate, ',')) 
		{ 
			tokens.push_back(intermediate); 
		}
		
		// Get and set status
		std::string status(tokens.front());
		if(status == "FAILURE_NOT_EXISTS") {
			reply.status = FAILURE_ALREADY_EXISTS;
			reply.port = -1;
			reply.num_member = -1;
		}
		else if(status == "SUCCESS") {
			reply.status = SUCCESS;
			reply.port = std::stoi(tokens.at(1));
			reply.num_member = std::stoi(tokens.at(2));
		}
		else {
			reply.status = FAILURE_UNKNOWN;
			reply.port = -1;
			reply.num_member = -1;
		}
		
    }

	// CREATE
    else if(strncmp(buffer, "CREATE ", 7) == 0) {
		char charStatus[BUFFER_LENGTH];
        rc = recv(sockfd, charStatus, sizeof(charStatus), 0);
        // test error rc < 0 or rc == 0 or   rc < sizeof(buffer
        if(rc < 0) {
            perror("Failed to recieve on socket");
        }

		// Get and set status
		std::string status(charStatus);
		if(status == "FAILURE_ALREADY_EXISTS") {
			reply.status = FAILURE_ALREADY_EXISTS;
		}
		else if(status == "FAILURE_INVALID") {
			reply.status = FAILURE_INVALID;
		}
		else if(status == "SUCCESS") {
			reply.status = SUCCESS;
		}
		else {
			reply.status = FAILURE_UNKNOWN;
		}
    }

	// DELETE
    else if(strncmp(buffer, "DELETE ", 7) == 0) {
		char charStatus[BUFFER_LENGTH];
        rc = recv(sockfd, charStatus, sizeof(charStatus), 0);
        // test error rc < 0 or rc == 0 or   rc < sizeof(buffer
        if(rc < 0) {
            perror("Failed to recieve on socket");
        }

		// Get and set status
		std::string status(charStatus);
		if(status == "FAILURE_NOT_EXISTS") {
			reply.status = FAILURE_NOT_EXISTS;
		}
		else if(status == "SUCCESS") {
			reply.status = SUCCESS;
		}
		else {
			reply.status = FAILURE_UNKNOWN;
		}
    }

	// LIST
    else if(strncmp(buffer, "LIST", 4) == 0) {
		char list[BUFFER_LENGTH];
        rc = recv(sockfd, list, sizeof(list), 0);
        // test error rc < 0 or rc == 0 or   rc < sizeof(buffer
        if(rc < 0) {
            perror("Failed to recieve on socket");
        }   

		// Populate reply struct
		reply.status = SUCCESS;
		strcpy(reply.list_room, list);
    }

	// Other for testing purposes
    else {
        reply.status = FAILURE_INVALID;
    }

	close(sockfd);
	return reply;
}

/* 
 * Get into the chat mode
 * 
 * @parameter host     host address
 * @parameter port     port
*/

struct sockStruct {
	int sock;
};


void* recvThreadFunction(void* socket) {
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	sockStruct *sockfd = static_cast<sockStruct*>(socket);
	char otherMsg[BUFFER_LENGTH];
	int err;
	for(;;) {
		err = recv(sockfd->sock, otherMsg, sizeof(otherMsg), 0);
		if(err < 0) {
			perror("Failed to recv msg\n");
		}
		display_message(otherMsg);
		std::string msg(otherMsg);
		if(msg == "Warning the chat room is now closing...\n") {
			process_chatmode_run = false;
			close(sockfd->sock);
			pthread_exit(NULL);
		}
	}
}

 
void* sendThreadFunction(void* socket) {
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	sockStruct *sockfd = static_cast<sockStruct*>(socket);
	char userMsg[BUFFER_LENGTH];
	int err;
	for(;;) {
		get_message(userMsg, BUFFER_LENGTH);
		std::string msg(userMsg);
		err = send(sockfd->sock, userMsg, sizeof(userMsg), 0);
		if(err < 0) {
			perror("Failed to send msg\n");
		}
	} 
}


void process_chatmode(const char* host, const int port)
{
	// ------------------------------------------------------------
	// GUIDE 1:
	// In order to join the chatroom, you are supposed to connect
	// to the server using host and port.
	// You may re-use the function "connect_to".
	// ------------------------------------------------------------
	//int sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	pthread_t recvThread;
	pthread_t sendThread;
	
	int sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd < 0) 
		perror("ERROR opening socket");
	
	// Populate the server connection data
	struct sockaddr_in serveraddr;

	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family      = AF_INET;
	serveraddr.sin_port        = htons(port);
	serveraddr.sin_addr.s_addr = inet_addr(host);

	// Connect to the server
	int err = connect(sockfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr));
	if(err == 0) { 
		printf("Connection Successfull\n");
	}
	// ------------------------------------------------------------
	// GUIDE 2:
	// Once the client have been connected to the server, we need
	// to get a message from the user and send it to server.
	// At the same time, the client should wait for a message from
	// the server.
	// ------------------------------------------------------------
	sockStruct *data = new sockStruct();
	data->sock = sockfd;
	
	pthread_create(&recvThread, NULL, recvThreadFunction, (void*)data);
	pthread_create(&sendThread, NULL, sendThreadFunction, (void*)data);
	while(process_chatmode_run) {
	}
	//pthread_cancel(recvThread);
	pthread_cancel(sendThread);
	return;
    // ------------------------------------------------------------
    // IMPORTANT NOTICE:
    // 1. To get a message from a user, you should use a function
    // "void get_message(char*, int);" in the interface.h file
    // 
    // 2. To print the messages from other members, you should use
    // the function "void display_message(char*)" in the interface.h
    //
    // 3. Once a user entered to one of chatrooms, there is no way
    //    to command mode where the user  enter other commands
    //    such as CREATE,DELETE,LIST.
    //    Don't have to worry about this situation, and you can 
    //    terminate the client program by pressing CTRL-C (SIGINT)
	// ------------------------------------------------------------
}

