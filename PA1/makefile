# makefile

all: client server

server: crsd.c
	g++ -g -w -Wall -O1 -std=c++11 -lpthread -o server crsd.c
	
client: crc.c
	g++ -g -w -Wall -O1 -std=c++11 -lpthread -o client crc.c

clean:
	rm -rf *.o *.fifo server client
