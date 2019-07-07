#ifndef PARROT_SERVER_H
#define PARROT_SERVER_H

#include <string>
#include "Common.h"

// Server class
class Server {
public:
    Server();

    void Init();

    void Start();

    void Close();

private:
    // Broad the msg to all client
    int SendBroadcastMessage(int clientfd);

    // server address msg
    struct sockaddr_in serverAddr;

    // create the socket listener
    int listener;

    // the return value of epoll_create function
    int epfd;

    // The client list
    std::list<int> client_list;
};

#endif
