/*
Caleb Edens - 822007959
Curtis Green - 422008537

Assignment #3.1
*/

/*///////////////////////////////////////////////////////////////////////////////////
Include Statements
*////////////////////////////////////////////////////////////////////////////////////
#include <iostream>
#include <memory>
#include <thread>
#include <vector>
#include <string>
#include <unistd.h>
#include <grpc++/grpc++.h>
#include "client.h"
#include "sns.grpc.pb.h"

/*///////////////////////////////////////////////////////////////////////////////////
Name Spaces
*////////////////////////////////////////////////////////////////////////////////////
using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;
using csce438::Message;
using csce438::Posting;
using csce438::ListReply;
using csce438::Request;
using csce438::ServerRequest;
using csce438::Reply;
using csce438::SNSService;

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


/*///////////////////////////////////////////////////////////////////////////////////
Client Class
*////////////////////////////////////////////////////////////////////////////////////
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
        std::string routerPort;
        std::string routerHostname;
        bool reconnected = false;
        // You can have an instance of the client stub
        // as a member variable.
        std::unique_ptr<SNSService::Stub> stub_;

        IReply Login();
        IReply Route();
        IReply List();
        IReply Follow(const std::string& username2);
        IReply UnFollow(const std::string& username2);
        void Timeline(const std::string& username);

};
/*///////////////////////////////////////////////////////////////////////////////////
Main
*////////////////////////////////////////////////////////////////////////////////////
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

/*///////////////////////////////////////////////////////////////////////////////////
Client Functions
*////////////////////////////////////////////////////////////////////////////////////

/*-----------------------------------------------------------------------------------
connect_to

Sets up initial connection with the server and tests the connection. 
---------------------------------------------*/
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
    std::string login_info = hostname + ":" + port;
    stub_ = std::unique_ptr<SNSService::Stub>(SNSService::NewStub(
               grpc::CreateChannel(
                    login_info, grpc::InsecureChannelCredentials())));

    IReply ire = Route();
    if(!ire.grpc_status.ok()) {
		std::cout << "Failed routing" << std::endl;
        return -1;
    }
	std::cout << "success routing" << std::endl;
    ire = Login();
    if(!ire.grpc_status.ok()) {
		std::cout << "Failed logging in" << std::endl;
        return -1;
    }
	std::cout << "success logging in" << std::endl;
    /*std::thread heart([&](){
        Reply pingReply;
        ServerRequest request;
        //ServerRequest routerRequest;
        
        request.set_port(port);
        while(1){
            ClientContext context;
            Status status;
            status = stub_->slavePing(&context, request, &pingReply);
			
            if(status.ok()){
                usleep(1000000);
            }
            else{
                //std::cout << "Connection to server lost, reconnecting..." << std::endl;
                port = routerPort;
                hostname = routerHostname;
                usleep(1000000);
                int i = connectTo();
                if (i>0){
					//std::cout << "Connection Complete" << std::endl;
				}
				else
					std::cout << "Connection failed" << std::endl;
                break;
            }
        }

    });
    heart.detach();*/
    return 1;
}

/*-----------------------------------------------------------------------------------
processCommand

Takes in command input (valid or not) and decides what to do with it. 
Good commands get parsed and sent to the server. Bad commands get an 
error message about the validity of the command. This manages
the command status the the client sees after a command is sent
as well.
---------------------------------------------*/
IReply Client::processCommand(std::string& input)
{
    IReply ire;
    std::size_t index = input.find_first_of(" ");
    if (index != std::string::npos) {
        std::string cmd = input.substr(0, index);

        std::string argument = input.substr(index+1, (input.length()-index));

        if (cmd == "FOLLOW") {
            ire =  Follow(argument);
        } else if(cmd == "UNFOLLOW") {
            ire = UnFollow(argument);
        }
    } else {
        if (input == "LIST") {
            ire = List();
        } else if (input == "TIMELINE") {
            ire.comm_status = SUCCESS;
            return ire;
        }
    }

    if (!ire.grpc_status.ok()){
        std::cout << "Reconnecting to server" << std::endl;
        port = routerPort;
        hostname = routerHostname;
        usleep(500000);
        int i = connectTo();
        if (i > 0){
            std::cout << "Connection Complete" << std::endl;
            ire = processCommand(input);
        }
        else{
            std::cout << "Connection failed" << std::endl;
        }

    }
	else if (ire.comm_status == SUCCESS){
		return ire;
	}
    else{
        ire.comm_status = FAILURE_INVALID;
    }
    return ire;
}

/*-----------------------------------------------------------------------------------
processTimeline

Hnadles the transition from command to timeline mode. Once that
transition is made, the only way to exit timeline mode is through
Ctrl + C
---------------------------------------------*/
void Client::processTimeline()
{
    Timeline(username);
}

/*-----------------------------------------------------------------------------------
List

Hnadles the list request
---------------------------------------------*/
IReply Client::List() {
    //Data being sent to the server
    Request request;
    request.set_username(username);

    //Container for the data from the server
    ListReply list_reply;

    //Context for the client
    ClientContext context;

    Status status = stub_->List(&context, request, &list_reply);
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
    }
    else{
        ire.comm_status = FAILURE_UNKNOWN;
    }
    return ire;
}

/*-----------------------------------------------------------------------------------
Follow

Hnadles the Follow request
---------------------------------------------*/        
IReply Client::Follow(const std::string& username2) {
    Request request;
    request.set_username(username);
    request.add_arguments(username2);

    Reply reply;
    ClientContext context;

    Status status = stub_->Follow(&context, request, &reply);
    IReply ire; ire.grpc_status = status;
    if (reply.msg() == "Follow Failed -- Invalid Username") {
        ire.comm_status = FAILURE_INVALID_USERNAME;
    } else if (reply.msg() == "Follow Failed -- Already Following User") {
        ire.comm_status = FAILURE_ALREADY_EXISTS;
    } else if (reply.msg() == "Follow Successful") {
        ire.comm_status = SUCCESS;
    } else {
        ire.comm_status = FAILURE_UNKNOWN;
    }
    return ire;
}

/*-----------------------------------------------------------------------------------
Unfollow

Hnadles the Unfollow request
---------------------------------------------*/
IReply Client::UnFollow(const std::string& username2) {
    Request request;

    request.set_username(username);
    request.add_arguments(username2);

    Reply reply;

    ClientContext context;

    Status status = stub_->UnFollow(&context, request, &reply);
    IReply ire;
    ire.grpc_status = status;
    if (reply.msg() == "UnFollow Failed -- Invalid Username") {
        ire.comm_status = FAILURE_INVALID_USERNAME;
    } else if (reply.msg() == "UnFollow Failed -- Not Following User") {
        ire.comm_status = FAILURE_INVALID_USERNAME;
    } else if (reply.msg() == "UnFollow Successful") {
        ire.comm_status = SUCCESS;
    } else {
        ire.comm_status = FAILURE_UNKNOWN;
    }

    return ire;
}

/*-----------------------------------------------------------------------------------
Login

Hnadles the initial registration of the client
---------------------------------------------*/
IReply Client::Login() {
    Request request;
    request.set_username(username);
    Reply reply;
    ClientContext context;

    Status status = stub_->Login(&context, request, &reply);

    IReply ire;
    ire.grpc_status = status;
    if (reply.msg() == "Invalid Username") {
        ire.comm_status = FAILURE_ALREADY_EXISTS;
    } else {
        ire.comm_status = SUCCESS;
    }
    //std::cout << username << " connected to server " << port << " from router " << routerPort << std::endl;
    return ire;
}

/*-----------------------------------------------------------------------------------
Route

Hendles the client redirection in the event of a server failure
---------------------------------------------*/
IReply Client::Route() {
    Request request;
    request.set_username(username);
    Reply reply;
    ClientContext context;

    Status status = stub_->Route(&context, request, &reply);

    IReply ire;
    ire.grpc_status = status;
    if (reply.msg() == "noLeader"){
        std::cout << "No leader Server Found" << std::endl;
        ire.comm_status = FAILURE_UNKNOWN;
    }
    else{
		//std::cout << "routerport = " << routerPort << std::endl;
        routerPort = port;
        routerHostname = hostname;
        //port = reply.msg();
        std::string ip = reply.msg();
		int index = ip.find(":");
		hostname = ip.substr(0, index);
		port = ip.substr(index+1, ip.length()-hostname.length());
        ire.comm_status = SUCCESS;
        std::string login_info = ip;
        //std::string login_info = hostname + ":" + port;
		std::cout << "new port stuff " << login_info << std::endl;
        stub_ = std::unique_ptr<SNSService::Stub>(SNSService::NewStub(
               grpc::CreateChannel(
                    login_info, grpc::InsecureChannelCredentials())));
    }
    return ire;
}

/*-----------------------------------------------------------------------------------
Timeline

Hnadles the Timeline request
---------------------------------------------*/
void Client::Timeline(const std::string& username) {
	
    while (true){
		std::cout<<"Top of loop" << std::endl;
        //Check if initial or reconnection
        if (reconnected){
			std::cout << "Reconnecting condition" << std::endl;   
            port = routerPort;
            hostname = routerHostname;
            usleep(500000);
            int i = connectTo();
            if (i>0){
                std::cout << "Connection Complete" << std::endl;
            }
            else{
                std::cout << "Connection failed" << std::endl;
            }
        }
        else{
            reconnected = true;
        }
        

        ClientContext context;
        /*auto myStream = std::shared_ptr<ClientReaderWriter<Posting, Posting>> stream(
                stub_->Timeline(&context));*/
    	std::shared_ptr<ClientReaderWriter<Posting, Posting>> stream = stub_->Timeline(&context);
    	
        //Thread used to read chat messages and send them to the server
        std::thread writer([username, stream]() {
                Posting p;
                p.set_content("connect");
                p.set_username(username);
    			//std::cout << "about to write" << std::endl;
                stream->Write(p);
                std::string msg;
                std::string user = username;
                while(1) {
                    std::getline(std::cin, msg);
                    p.set_content(msg);
                    p.set_username(user);
                    if (stream->Write(p) == false){
						break;
					}
                }
                stream->WritesDone();
                });

        std::thread reader([&]() {
                Posting p;
                while(stream->Read(&p)){
                    std::cout << std::endl;
                    std::cout << p.content() << std::endl;
                }
                });
		std::cout << "Exited threads" << std::endl;
        //Wait for the threads to finish
        reader.join();
		writer.join();
		std::cout << "Joined read & write" << std::endl;

    }
}

