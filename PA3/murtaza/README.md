
Compile the code using the provided makefile:

    make

To clear the directory (and remove .txt files):
   
    make clean

To run the server on port 3010 as routing server:

    ./tsd -p 3010 -r routing

To run the server on port 3010 as available server:

    ./tsd -p 3010 -r available

To run the server on port 3010 as master server:

    ./tsd -p 3010 -r master

To run the server on port 3010 as slave server:

    ./tsd -p 3010 -r slave

To run the client  

    ./tsc -h host_addr -p 3010 -u user1

