
Compile the code using the provided makefile:

    make

To clear the directory:
   
    make clean

To run the router on hostname localhost and port 3010:

    ./tsd -t router -p 3010 -h router_host_addr
    
To run the master & slave on hostname localhost and port 4000 with router port 3010:

    ./tsd -t master -q router_host_addr -r 3010 -s slave -h host_addr -p 4000
    
To run the client to connect to router port 3010

    ./tsc -h host_addr -p 3010 -u user1
 
Note to grader:
 * Reconnecting in timeline works, but it requires each client to send 1 message before normal behavior comes back
 * Killing the router server will stop the slave servers from bringing their masters back to life
 * Data is written to data.csv, deleting the file will remove clear the current persistent data

