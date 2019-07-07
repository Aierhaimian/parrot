#ifndef PARROT_CLIENT_H
#define PARROT_CLIENT_H

#include <string>
#include "Common.h"

class Client {
public:
    Client();

    void Connect();

    void Close();

    void Start();

private:
    int sock;

    // current pid
    int pid;

    int epfd;

    // create pipe: 
    // fd[0] is used to read father process
    // fd[1] is used to write child process
    int pipe_fd[2];

    bool isClientwork;

    Msg msg;

    char send_buf[BUFFER_SIZE];
    char recv_buf[BUFFER_SIZE];

    // The server ip + port
    struct sockaddr_in serverAddr;
};

#endif
