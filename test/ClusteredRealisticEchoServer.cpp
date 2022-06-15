/*
* EchoServer.cpp
*
*  Created on: Apr 19, 2018
*      Authors: Saman Barghi, Erfan Sharafzadeh
*/


#include <uThreads/uThreads.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <iostream>
#include <random>
#include <sys/time.h>
#include <time.h>

using namespace std;
std::default_random_engine generator;

#define MAXIMUM_THREADS_PER_CLUSTER 8

void echo(void* c){

    Connection* cconn = (Connection*)c;
    std::vector<char>  msg(150);
    int res;
    double now;
    double difference;
    struct timeval my_time, moment;
    std::discrete_distribution<int> distribution {13000, 13000,13000,13000,13000, 5300,5300,5300,5300,5300,
        2500,2500,2500,2500,2500,1550,1550,1550,1550,1550,
        1200,1200,1200,1200,1200,850,850,850,850,850,
        700,700,700,700,700,550,550,550,550,550,
        400,400,400,400,400,350,350,350,350,350,
        300,300,300,300,300,250,250,250,250,250,
        200,200,200,200,200,180,180,180,180,180,
        175,175,175,175,175,150,150,150,150,150,
        140,140,140,140,140,130,130,130,130,130,
        125,125,125,125,125,122,122,122,122,122,
        120,120,120,120,120,100,100,100,100,100,
        100,100,100,100,100,90,90,90,90,90,
        80,80,80,80,80,75,75,75,75,75,
        70,70,70,70,70,65,65,65,65,65,
        60,60,60,60,60,55,55,55,55,55,
        50,50,50,50,50,45,45,45,45,45,
        40,40,40,40,40,35,35,35,35,35,
        30,30,30,30,30,25,25,25,25,25,
        20,20,20,20,20,20,20,20,20,20,
        15,15,15,15,15,125,125,125,125,125
    };
    while( (res = cconn->recv(msg.data(), msg.size(), 0)) > 0){
        do {
            now = distribution(generator)*1000.0;
            //printf("now: %f\n", now);
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
    Cluster* clusters[thread_count];
    clusters[0] = &defaultCluster;
    for(size_t i=1; i < thread_count-1; i++){
        Cluster* c = new Cluster();
        clusters[i] = c;
        kThreads[i] = new kThread(*c);
    }
    Cluster* c = new Cluster();
    clusters[thread_count-1] = c;
    kThreads[thread_count-1] = new kThread(*c);


    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htons(INADDR_ANY);
    servaddr.sin_port = htons(serverPort);

    //Creating the socket
    try{
        Connection sconn(AF_INET, SOCK_STREAM , 0);
        if(sconn.nodelay() < 0){
            cerr << "Could set socket nodelay" << endl;
            return 1;
        }
        int i=1;
        setsockopt( sconn.getFd(), IPPROTO_TCP, TCP_NODELAY, (void *)&i, sizeof(i));
        if( sconn.bind((struct sockaddr *) &servaddr, sizeof(servaddr)) < 0){
            cerr << "Could not bind to 8888" << endl;
            return 1;
        }
        sconn.listen(1000);
        int c_index = 0;
        for(;;){
            Connection* cconn  = sconn.accept((struct sockaddr*)nullptr, nullptr);
            cout << "Accepted" << endl;
            uThread::create()->start(*clusters[(c_index++)%thread_count], (void*)echo, (void*)cconn);
            cout << "uThread Created" << endl;
        }
    }catch (std::system_error& error){
        std::cout << "Error: " << error.code() << " - " << error.what() << '\n';
    }

    return 0;
}
