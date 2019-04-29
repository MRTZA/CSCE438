
Compile the code using the provided makefile:

    make

To clear the directory (and remove .txt files):
   
    make clean

To run the servers:

    sh startup.sh
    (It will prompt you to ask if you are starting a router or just a normal master/slave )

To run the client:  

    ./tsc -h host_addr -p 3010 -u user1

