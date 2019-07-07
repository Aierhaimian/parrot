#include <iostream>

#include "Client.h"

Client::Client() {
    serverAddr.sin_family           = PF_INET;
    serverAddr.sin_port             = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr      = inet_addr(SERVER_IP);
    bzero(&(serverAddr.sin_zero), sizeof(serverAddr.sin_zero));

    sock = 0;

    pid = 0;

    isClientwork = true;

    epfd = 0;
}

void Client::Connect() {
    std::cout << "Connect Server: " << SERVER_IP << ":" << SERVER_PORT << std::endl;

    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(-1);
    }

    if (connect(sock, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr)) < 0) {
        perror("connect");
        exit(-1);
    }

    if (pipe(pipe_fd) < 0) {
        perror("pipe");
        exit(-1);
    }

    if ((epfd = epoll_create(EPOLL_SIZE)) < 0) {
        perror("epoll_create");
        exit(-1);
    }

    addFd(epfd, sock, true);
    addFd(epfd, pipe_fd[0], true);
}

void Client::Close() {
    if (pid) {
        // close father process's pipe and sock
        close(pipe_fd[0]);
        close(sock);
    }
    else {
        // close child's pipe
        close(pipe_fd[1]);
    }
}

void Client::start() {
    static struct epoll_event events[2];

    Connect();

    if ((pid = fork()) < 0) {
        perror("fork");
        exit(-1);
    }else if (pid ==0) {
        // The child process
        // The child process write pipe, must close read first
        close(pipe_fd[0]);

        std::cout << "Please input 'exit' to exit the chat room" << std::endl;
        std::cout << "\\ + ClientID to private chat" << std::endl;

        while (isClientwork) {
            memset(msg.content, 0, sizeof(msg.content));
            fgets(msg.content, BUFFER_SIZE, stdin);

            if (strncasecmp(msg.content, EXIT, strlen(EXIT)) == 0) {
                isClientwork = 0;
            }
            // The child write pipe
            else {
                memset(send_buf, 0, BUFFER_SIZE);
                memcpy(send_buf, &msg, sizeof(msg));
                if (write(pipe_fd[1], send_buf, sizeof(send_buf)) < 0) {
                    perror("fork error");
                    exit(-1);
                }
            }
        }
    }
    else {
        // The father process
        // The father process read pipe, must close write first
        close(pipe_fd[1]);

        while (isClientwork) {
            int epoll_events_count = epoll_wait(epfd, events, 2, -1);

            for (int i=0; i<epoll_events_count; i++)
            {
                memset(recv_buf, 0, sizeof(recv_buf));

                if (events[i].data.fd == sock) {
                    int ret = recv(sock, recv_buf, BUFFER_SIZE, 0);

                    memset(&msg, 0, sizeof(msg));
                    memcpy(&msg, recv_buf, sizof(msg));

                    if (ret == 0) {
                        std::cout << "Server closed connection: " << sock << std::endl;
                        close(sock);
                        isClientwork = false;
                    }
                    else {
                        std::cout << msg.content << std::endl;
                    }
                }
                else {
                    int ret = read(events[i].data.fd, recv_buf, BUFFER_SIZE);

                    if (ret == 0)
                        isClientwork = false;
                    else {
                        send(sock, recv_buf, sizeof(recv_buf), 0);
                    }
                }
            }
        }
    }
    Close();
}
