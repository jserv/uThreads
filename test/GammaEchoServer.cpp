/*
 * EchoServer.cpp
 *
 *  Created on: Apr 19, 2018
 *      Authors: Saman Barghi, Erfan Sharafzadeh
 */


#include "uThreads.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <iostream>
#include <random>
#include <sys/time.h>
#include <time.h>

using namespace std;
std::default_random_engine generator;
int k = 1.0, theta = 1.0;

#define MAXIMUM_THREADS_PER_CLUSTER 8

void echo(void* c){

     Connection* cconn = (Connection*)c;
     std::vector<char>  msg(1025);
     int res;
     double now;
     double difference;
     struct timeval my_time, moment;
     std::gamma_distribution<double> distribution(k, theta);
     while( (res = cconn->recv(msg.data(), msg.size(), 0)) > 0){
         do {
             now = distribution(generator);
             printf("now: %f\n", now);
         }
         while (now < 0);
    	 gettimeofday(&my_time, 0);
    	 //printf("it will take %f us\n", now);
    	 do {
    	 	 gettimeofday(&moment, 0);
    	 	 difference = (moment.tv_sec - my_time.tv_sec)*1000000 + (moment.tv_usec - my_time.tv_usec);
             //printf("then: %f\n", difference);
    	 } while(difference < now);
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

    if (argc != 5) {
      cerr << "Usage: " << argv[0] << " <Server Port> <threads> <k> <theta>" << endl;
      exit(1);
    }
    uint serverPort = atoi(argv[1]);

	size_t thread_count = atoi(argv[2]);
    k = atof(argv[3]);
    theta = atof(argv[4]);
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
