#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <string>
#include <vector>
#include <pthread.h>
#include <iostream>
#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/poll.h>
#include "interface.h"

#define SERVER_PORT     3005
#define SERVER_ADDR     "127.0.0.1"
#define BUFFER_LENGTH    250
#define NAME_LIMIT        32
#define MAX_CHATROOMS     16
#define FALSE              0
#define DEBUG              1
#define MAX_BACKLOG		  10
#define CHAT_LIMIT		  10

using std::cout;
using std::endl;

// struct to store chatroom data
struct chatroom_data {
    int sockfd;
    int port;
	int num_members;
    char name[NAME_LIMIT];
    int isActive = 0; // 0 - Inactive, 1 - Active
    struct sockaddr_in serveraddr;
	pthread_t chatroom_thread;
};

// Global Variables
std::vector<chatroom_data> cdata;
std::vector<int> availablePorts;

// Creates the master socket
int passiveTCPsock(int port, int backlog, bool isBlocking) {
	
	// Create the master socket assumes IPv4 and TCP connection
	int m_sock;
	int rc = 1;
	struct sockaddr_in serveraddr;
	if(isBlocking) {
		m_sock = socket(AF_INET, SOCK_STREAM, 0);
	} else {
		m_sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	}
	// test error m_sock< 0
	if(m_sock < 0) {
		perror("Failed to create socket");
	} 
	
	// Creates serveraddr with default ip and port
	// Might want to change this to be more flexible
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family      = AF_INET;
	serveraddr.sin_port        = htons(port);
	serveraddr.sin_addr.s_addr = inet_addr(SERVER_ADDR);
	
	// Bind the master socket to the serveraddr
	rc = bind(m_sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
	// test error rc < 0
	if(rc < 0) {
		perror("Failed to bind socket");
	}
	
	// Listen for incoming connections with a limit of #backlog waiting connections
	rc = listen(m_sock, backlog);
	// test error rc< 0
	if(rc < 0) {
		perror("Failed to listen on socket");
	} 
	
	//Returned master socket will contain connections that we are able to wait on.
	return m_sock;
}

// Handles all the chat rooms and whatnot
void* chat_room_thread(void* chat_room_information) {
	//Get our chat room information
	struct chatroom_data* chatInfo = (chatroom_data*) chat_room_information;
	
	// Allow the thread to be cancelable for when DELETE gets called on it
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	
	// Get the chatroom name in string form
	std::string chatroomName(chatInfo->name);
	
	//Create a passive TCP socket for this specific chat room
	char buffer[BUFFER_LENGTH];
	int m_sock, s_sock, rc, sd;
	int fdmax = -1;
	std::vector<int> sockets;
	
	cout << "Launching chatroom: " << chatroomName << endl;
	
	m_sock = passiveTCPsock(chatInfo->port, MAX_BACKLOG, false);
	
	cout << "Master socket created" << endl;
	
	fd_set readset, readset_backup;
	
	for(;;) {
		
		if(!chatInfo->isActive) {
			for(int i = 0; i < cdata.size(); i++) {
				std::string name(cdata.at(i).name);
				if(chatroomName == name) {
					int port = cdata.at(i).port;
					// Close the connection
					close(cdata.at(i).sockfd);
					// Remove the chat room from the list
					cdata.erase(cdata.begin() + i);
					// Make the port available again
					availablePorts.push_back(port);
				}
			}
			for(auto & socket : sockets) {
				strcpy(buffer, "Warning the chat room is now closing...\n");
				rc = send(socket, buffer, sizeof(buffer), 0);
				if(rc < 0) {
					perror("Failed to send\n");
				}
				close(socket);
				pthread_exit(NULL);
				return NULL;
			}
		}
		
		FD_ZERO(&readset);
		
		FD_SET(m_sock, &readset);
		fdmax = m_sock;
		
		for(auto & sock : sockets) {
			sd = sock;
			if(sd > 0)
				FD_SET(sd, &readset);
			
			if(sd > fdmax)
				fdmax = sd;
		}
		
		rc = select(fdmax + 1, &readset, NULL, NULL, NULL);
		
		if(rc < 0 && errno != EINTR) {
			perror("select error occurred\n");
		}
		
		if(FD_ISSET(m_sock, &readset)) {
			s_sock = accept(m_sock, NULL, NULL);
			if (s_sock == EAGAIN || s_sock == EWOULDBLOCK) { 
			// There are no new connections to accept() on
			} else { 
				// We have new connection to add
				// Add the new socket to our read set
				cout << "Adding a connection" << endl;
				sockets.push_back(s_sock);
			}
		}
		
		// For each socket check if it is set
		//cout << "Checking is sockets have stuff to read" << endl;
		for(auto & socket1 : sockets ) {
			if(FD_ISSET(socket1, &readset)) {
				// If the socket is set then receive it's msg and send it on all other sockets
				rc = recv(socket1, buffer, sizeof(buffer), 0);
				if(rc < 0) {
					perror("Failed to receive message from the clients");
				}
				std::string message(buffer);
				message += "\n";
				cout << "Server Message: " << message;
				
				for( auto & socket2 : sockets) {
					if( socket1 != socket2) {
						strcpy(buffer, message.c_str());
						rc = send(socket2, buffer, sizeof(buffer), 0);
						if(rc < 0) {
							perror("Failed to send\n");
						}
					}
				}
			}
		}
	}

	pthread_exit(NULL);
}

/* 0 - Failure, 1 - Success */
// CREATE
int createCommand(char buffer[BUFFER_LENGTH], int s_sock) {
    if(cdata.size() < MAX_CHATROOMS) { // Can allow more chatrooms
        // Get the chatroom name
        char subbuff[NAME_LIMIT];
        memcpy(subbuff, &buffer[7], NAME_LIMIT-1);
        subbuff[NAME_LIMIT-1] = '\0';

        // Convert name to string
        std::string str(subbuff);
        // Check if chatroom already exists
        for(int i = 0; i < cdata.size(); i++) {
            std::string name(cdata.at(i).name);
            if(str == name) {
                std::string msg = "FAILURE_ALREADY_EXISTS";
                char msgChar[BUFFER_LENGTH];
                strcpy(msgChar, msg.c_str());

                int rc = send(s_sock, msgChar, sizeof(msgChar), 0);
                // test error rc < 0
                if(rc < 0) {
                    perror("Failed to reply: CREATE Command");
                }

                printf("Chatroom name already exists\n");

                return 0;
            }
        }

        /* Chatroom creation was sucessful */ 
        // Create the socket and slave port
        int sockfd = socket(AF_INET, SOCK_STREAM, 0); 
        if (sockfd < 0) 
            perror("ERROR opening socket");

        // Find an available port and delete it
        int port = availablePorts.at(0);
        availablePorts.erase(availablePorts.begin());

        struct sockaddr_in serveraddr;
        memset(&serveraddr, 0, sizeof(serveraddr));
        serveraddr.sin_family      = AF_INET;
        serveraddr.sin_port        = htons(port);
        serveraddr.sin_addr.s_addr = inet_addr(SERVER_ADDR);

        chatroom_data temp;
        temp.sockfd = sockfd;
        temp.port = port;
        strcpy(temp.name, subbuff);
        temp.isActive = 1;
        temp.serveraddr = serveraddr;
		temp.num_members = 0;

        cdata.push_back(temp);

        printf("Chatroom created Successfully\n");

        std::string msg = "SUCCESS";
        char msgChar[BUFFER_LENGTH];
        strcpy(msgChar, msg.c_str());

        int rc = send(s_sock, msgChar, sizeof(msgChar), 0);
        // test error rc < 0
        if(rc < 0) {
            perror("Failed to reply: CREATE Command");
        }
		
		//If all of the above succeeds then we want to launch the chatroom thread here
		pthread_create(&cdata.back().chatroom_thread, NULL, chat_room_thread, &cdata.back()); 

    } else {
        // TODO: Error Handling
        std::string msg = "FAILURE_INVALID";
        char msgChar[BUFFER_LENGTH];
        strcpy(msgChar, msg.c_str());

        int rc = send(s_sock, msgChar, sizeof(msgChar), 0);
        // test error rc < 0
        if(rc < 0) {
            perror("Failed to reply: CREATE Command");
        }

        printf("Max chatrooms already created\n");
    }
    
    return 1;
}

// JOIN
int joinCommand(char buffer[BUFFER_LENGTH], int s_sock) {
	// Get the chatroom name
	char subbuff[NAME_LIMIT];
	memcpy(subbuff, &buffer[5], NAME_LIMIT-1);
	subbuff[NAME_LIMIT-1] = '\0';

	// Convert name to string
	std::string targetName(subbuff);
	
	//Check if that name even exists
	int targetPos = -1;
	for(int i = 0; i < cdata.size(); i++) {
		std::string chatroomName(cdata.at(i).name);
		if(chatroomName == targetName) {
			targetPos = i;
		}
	}
	
	// if the room exists we can join it
	
	if(targetPos > -1) {
		// send back a reply with the information that the client needs to connect
		// success status, port, num_members 
		cdata.at(targetPos).num_members += 1;
		std::string msg("SUCCESS");
		msg += ",";
		msg += std::to_string(cdata.at(targetPos).port);
		msg += ",";
		msg += std::to_string(cdata.at(targetPos).num_members);
		cout << msg << endl;
		
		char msgChar[BUFFER_LENGTH];
		strcpy(msgChar, msg.c_str());
		
		int rc = send(s_sock, msgChar, sizeof(msgChar), 0);
		
	} else { //Chat room isn't found
		printf("Chat room cannot be joined...\n");

        std::string msg = "FAILURE_NOT_EXISTS";
        char msgChar[BUFFER_LENGTH];
        strcpy(msgChar, msg.c_str());

        int rc = send(s_sock, msgChar, sizeof(msgChar), 0);
        // test error rc < 0
        if(rc < 0) {
            perror("Failed to reply: LIST Command");
        }
	}
	
    return 1;
}

// DELETE
int deleteCommand(char buffer[BUFFER_LENGTH], int s_sock) {
    // Initialize reply to failure
    std::string msgFail = "FAILURE_NOT_EXISTS";
    char msgFailChar[BUFFER_LENGTH];
    strcpy(msgFailChar, msgFail.c_str());

    // check if there are any chatrooms to even delete
    if(cdata.size() == 0) {
        int rc = send(s_sock, msgFailChar, sizeof(msgFailChar), 0);
        // test error rc < 0
        if(rc < 0) {
            perror("Failed to reply: DELETE Command");
        }
        printf("No chatrooms exist\n");
        return 0;
    }

    // Get the chatroom name
    char subbuff[NAME_LIMIT];
    memcpy(subbuff, &buffer[7], NAME_LIMIT-1);
    subbuff[NAME_LIMIT-1] = '\0';
    // Convert name to string
    std::string str(subbuff);

    for(int i = 0; i < cdata.size(); i++) {
        std::string name(cdata.at(i).name);
        if(str == name) {
			cdata.at(i).isActive = false;
            if(cdata.at(i).num_members == 0) {
				
				int port = cdata.at(i).port;
				// Close the connection
				close(cdata.at(i).sockfd);
				// Remove the chat room from the list
				cdata.erase(cdata.begin() + i);
				// Make the port available again
				availablePorts.push_back(port);
			}
            std::string msg = "SUCCESS";
            char msgChar[BUFFER_LENGTH];
            strcpy(msgChar, msg.c_str());
			

            int rc = send(s_sock, msgChar, sizeof(msgChar), 0);
            // test error rc < 0
            if(rc < 0) {
                perror("Failed to reply: CREATE Command");
            }
            printf("Chatroom deleted successfully\n");
            return 1;
        }
    }

    int rc = send(s_sock, msgFailChar, sizeof(msgFailChar), 0);
    // test error rc < 0
    if(rc < 0) {
        perror("Failed to reply: DELETE Command");
    }
    printf("Chatroom of that name does not exist\n");
    return 0;
}

// LIST
int listCommand(char buffer[BUFFER_LENGTH], int s_sock) {
    // Check if there are any chatrooms
    if(cdata.size() > 0) {
        // Build reply string
        std::string msg = "";
        for(int i = 0; i < cdata.size(); i++) {
            if(cdata.at(i).isActive == 1) {
                std::string tp(cdata[i].name);
                if(i < cdata.size()-1) { msg += tp + ","; }
                else { msg += tp; }
            }
        }
        
        // Convert back to char[]
        char msgChar[BUFFER_LENGTH];
        strcpy(msgChar, msg.c_str());   

        printf("LIST: ");
        int loop = 0;
        while(msgChar[loop] != '\0') {
            printf("%c", msgChar[loop]);
            loop++;
        }
        printf("\n");

        int rc = send(s_sock, msgChar, sizeof(msgChar), 0);
        // test error rc < 0
        if(rc < 0) {
            perror("Failed to reply: LIST Command");
        }

    } else {
        printf("No Active Chat Rooms...\n");

        std::string msg = "No Active Chat Rooms";
        char msgChar[BUFFER_LENGTH];
        strcpy(msgChar, msg.c_str());

        int rc = send(s_sock, msgChar, sizeof(msgChar), 0);
        // test error rc < 0
        if(rc < 0) {
            perror("Failed to reply: LIST Command");
        }

    }
    return 1;
}

// Identifies what command the client sent
int identifyCommand(char buffer[BUFFER_LENGTH], int s_sock) {

    if(strncmp(buffer, "JOIN ", 5) == 0) {
        joinCommand(buffer, s_sock);
    }
    else if(strncmp(buffer, "CREATE ", 7) == 0) {
        createCommand(buffer, s_sock);
    }
    else if(strncmp(buffer, "DELETE ", 7) == 0) {
        deleteCommand(buffer, s_sock);
    }
    else if(strncmp(buffer, "LIST", 4) == 0) {
        listCommand(buffer, s_sock);
    }
    else {
        // Error Handling:
        // no need to send b/c already handled on client end
        printf("Unknown Command\n");
        return 0;
    }
    return 1;
}

int main() {
    int    m_sock = -1, s_sock = -1;
    int    rc, length, on = 1;
    char   buffer[BUFFER_LENGTH];
    struct timeval timeout;
    struct sockaddr_in serveraddr;
	std::vector<pthread_t> chatroom_threads;

    // Populate available ports w/ 3006->3022
    for(int i = 0; i < MAX_CHATROOMS; i++) {
        availablePorts.push_back(3006+i);
    }
	
	// Creates a master socket that listens for connections up to argument amount
	m_sock = passiveTCPsock(SERVER_PORT, MAX_BACKLOG, true);
	
	printf("Ready for client connect().\n");
	
	// Main server loop
	for(;;) {
		// Create our slave socket
		s_sock = accept(m_sock, NULL, NULL);
		if(s_sock < 0) {
			perror("Failed to accept socket");
		}
		
		// Receive information from client
		rc = recv(s_sock, buffer, sizeof(buffer), 0);
		// test error rc < 0
		if(rc < 0) {
			perror("Failed to receive on socket");
		}
		
		if (DEBUG) {
			printf("Printing...\n");

			int loop = 0;
			while(buffer[loop] != '\0') {
				printf("%c", buffer[loop]);
				loop++;
			}

			printf("\n");
		}
		
		// Always identify the command with the slave socket
		identifyCommand(buffer, s_sock);
		
		close(s_sock);
	
	}

	
    if (m_sock != -1)
    close(m_sock);
    if (s_sock != -1)
    close(s_sock);

    return 0;
}

