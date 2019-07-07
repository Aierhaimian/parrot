#ifndef PARROT_COMMON_H
#define PARROT_COMMON_H

#include <iostream>
#include <list>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>

// The initial server ip
#define SERVER_IP "127.0.0.1"

// The server port number
#define SERVER_PORT 6666

#define BACKLOG 10

// The maximum number of handles supported by epoll
#define EPOLL_SIZE 5000

// Buffer size
#define BUFFER_SIZE 0xFFFF

// Welcome message
#define SERVER_WELCOME "Welcome to the parrot chat room! Your chat ID is: Client #%d"

// The prefix of messages that the other client received
#define SERVER_MSG "Client %d say >> %s"
#define SERVER_PRIVATE_MSG "Client %d say to you privately >> %s"
#define SERVER_PRIVATE_ERROR_MSG "Client %d is not in the chat room now~"

// Exit the parrot
#define EXIT "EXIT"

// You are the only one in the chat room
#define CAUTION "There is only one in the chat room now!"

// Regsiter the new fd in epollfd
static void addFd(int epollfd, int fd, bool enable_et)
{
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN;
    if (enable_et)
        ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOL_CTL_ADD, fd, $ev);

    // Set the socket is non-blocked mode
    // The socket return immediately，and the thread in
    // which the function resides continues to run regardless
    // of whether the I/O is complete.
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);

    std::cout<< "The fd added to epoll!" << std::endl;
}

// The MSG class
// Transfer between server and client
struct Msg
{
    int type;
    int fromID;
    int toID;
    char content[BUFFER_SIZE];
};

#endif
