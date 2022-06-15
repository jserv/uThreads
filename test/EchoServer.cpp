/*
 * EchoServer.cpp
 *
 *  Created on: Jan 28, 2016
 *      Author: Saman Barghi
 */


#include "uThreads.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <iostream>

using namespace std;

#define MAXIMUM_THREADS_PER_CLUSTER 8

void echo(void* c){

     Connection* cconn = (Connection*)c;
     std::vector<char>  msg(1025);
     int res;

     while( (res = cconn->recv(msg.data(), msg.size(), 0)) > 0){
         cconn->write(msg.data(), res);
     }
     if(res == 0){
         cout << "Client closed the connection" << endl;
         cconn->close();
         delete cconn;
     }
     if(res < 0){
         cerr << "Receiving from client failed!" << endl;
         cconn->close();
         delete cconn;
     }
}

int main(int argc, char* argv[]) {

    if (argc != 3) {
      cerr << "Usage: " << argv[0] << " <Server Port> <threads>" << endl;
      exit(1);
    }
    uint serverPort = atoi(argv[1]);
	
	size_t thread_count = atoi(argv[2]);
    //Create clusters based on MAXIMUM_THREADS_PER_CLUSTER
    size_t cluster_count = (thread_count/(MAXIMUM_THREADS_PER_CLUSTER+1))+1;

    Cluster& defaultCluster = Cluster::getDefaultCluster();
    //Create kThreads, default thread is already started --> i=1
    kThread* kThreads[thread_count-1];
    for(size_t i=1; i < thread_count-1; i++)
        kThreads[i] = new kThread(defaultCluster);

    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htons(INADDR_ANY);
    servaddr.sin_port = htons(serverPort);

    //Creating the socket
    try{
        Connection sconn(AF_INET, SOCK_STREAM , 0);
        if( sconn.bind((struct sockaddr *) &servaddr, sizeof(servaddr)) < 0){
            cerr << "Could not bind to 8888" << endl;
            return 1;
        }
        sconn.listen(10);
        for(;;){
            Connection* cconn  = sconn.accept((struct sockaddr*)nullptr, nullptr);
            cout << "Accepted" << endl;
            uThread::create()->start(defaultCluster, (void*)echo, (void*)cconn);
        }
    }catch (std::system_error& error){
            std::cout << "Error: " << error.code() << " - " << error.what() << '\n';
    }

    return 0;
}
