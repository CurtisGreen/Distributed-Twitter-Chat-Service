# Distributed-Twitter-Chat-Service
### Description
Follow other users to see their posts. Write posts yourself and all followers will be able to see them in real time.

### Notes
* Server remembers all users, posts, and followers even after exiting
* Servers can be launched from different locations
* Client usage and memory load is distributed evenly
* Each username is assumed to be unique, launching with the same name will show that user's posts and followers
* Made using C++ and gRPC

### Setup and Run
Compile the code using the provided makefile:

    make

To clear the directory:
   
    make clean

To run the router on hostname localhost and port 3010:

    ./tsd -t router -p 3010 -h router_host_addr
    
To run the master & slave with router port 3010:

    ./tsd -t master -q router_host_addr -r 3010 -s slave -h host_addr -p 4000
    
To run the client to connect to router port 3010

    ./tsc -h host_addr -p 3010 -u user1
 
### Notes:
 * Reconnecting in timeline works, but it requires the clients to send two messages each before normal behavior comes back
 * Killing the router server will stop the slave servers from bringing their masters back to life
 * Data is written to data.csv, deleting the file will remove clear the current persistent data

