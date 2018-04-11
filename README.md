
Compile the code using the provided makefile:

    make

To clear the directory:
   
    make clean

To run the router on hostname localhost and port 3010:

    ./tsd -t router -p 3010 -h localhost
    
To run the master & slave on hostname localhost and port 4000 with router port 3010:

    ./tsd -t master -p 4000 -h localhost -s slave -r 3010
    
To run the client to connect to router port 3010

    ./tsc -h host_addr -p 3010 -u user1
 
Note to grader:
 * Client can reconnect to new server while in COMMAND MODE if current server dies
 * Cannot currently reconnect to new server if in TIMELINE MODE
 * Data is written to data.csv, deleting the file will remove clear the current persistent data

