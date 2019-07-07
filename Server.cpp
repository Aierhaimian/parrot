#include <iostream>

#include "Server.h"

Server::Server() {
    serverAddr.sin_family       = PF_INET;
    serverAddr.sin_port         = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr  = inet_addr(SERVER_IP);
    bzero(&(serverAddr.sin_zero),sizeof(serverAddr.sin_zero));

    listener = 0;

    epfd = 0;
}

void Server::Init() {
    std::cout << "Init Prrot Server..." << std::endl;
    if ((listener = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(-1);
    }

    if (bind(listener, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr)) < 0) {
        perror("bind");
        exit(-1);
    }

    if (listen(listener, BACKLOG) < 0) {
        perror("listen");
        exit(-1);
    }

    std::cout << "Start to listen: " << SERVER_IP << std::endl;

    if ((epfd = epoll_create(EPOLL_SIZE)) < 0) {
        perror("epoll_create");
        exit(-1);
    }

    addFd(epfd, listener, true);
}

void Server::Close() {
    close(listener);

    close(epfd);
}

int Server::SendBroadcastMessage(int clientfd) {
    // buf[BUFFER_SIZE] receive new messages
    // message[BUFFER_SIZE] save formatted messages
    char recv_buf[BUFFER_SIZE];
    char send_buf[BUFFER_SIZE];
    Msg msg;
    bzero(recv_buf, BUFFER_SIZE);

    // receive new message
    std::cout << "receive from client (clientID = " << clientfd << ")" << std::endl;
    int len = recv(clientfd, recv_buf, BUFFER_SIZE, 0);

    memset(&msg, 0, sizeof(msg));
    memcpy(&msg, recv_buf, sizeof(msg));

    msg.fromID = clientfd;
    if (msg.content[0] == '\\' && isdigit(msg.content[1])) {
        msg.type = 1;
        msg.toID = msg.content[1] - '0';
        memcpy(msg.content, msg.content+2, sizeof(msg.content));
    }
    else
        msg.type = 0;

    // if client is close connection
    if (len == 0) {
        close(clientfd);

        client_list.remove(clientfd);
        std::cout << "ClientID = " << clientfd << "closed." << std::endl;
        std::cout << "now there are " << client_list.size()
            << "client in the chat room" << std::endl;
    }
    // send msg to all client
    else {
        if (client_list.size() == 1) {
            memcpy(&msg.content, CAUTION, sizeof(msg.content));
            bzero(send_buf, BUFFER_SIZE);
            memcpy(send_buf,&msg, sizeof(msg));
            send(clientfd, send_buf, sizeof(send_buf), 0);
            return len;
        }

        char format_message[BUFFER_SIZE];
        // group chat
        if (msg.type == 0) {
            sprintf(format_message, SERVER_MSG, clientfd, msg.content);
            memcpy(msg.content, format_message, BUFFER_SIZE);

            std::list<int>::iterator it;
            for (it = client_list.begin(); it!=client_list.end(); it++) {
                if (*it != clientfd) {
                    bzero(send_buf, BUFFER_SIZE);
                    memcpy(send_buf, &msg, sizeof(msg));
                    if (send(*it, send_buf, sizeof(send_buf), 0) < 0) {
                        return -1;
                    }
                }
            }
        }
        // single chat
        if (msg.type == 1) {
            bool private_offline = true;
            sprintf(format_message, SERVER_PRIVATE_MSG, clientfd, msg.content);
            memcpy(msg.content, format_message, BUFFER_SIZE);

            std::list<int>::iterator it;
            for (it = client_list.begin(); it != client_list.end(); it++) {
                if (*it == msg.toID) {
                    private_offline = false;
                    bzero(send_buf, BUFFER_SIZE);
                    memcpy(send_buf, &msg, sizeof(msg));
                    if (send(*it, send_buf, sizeof(send_buf), 0) < 0) {
                        return -1;
                    }
                }
            }
            // if the another client if offline
            if (private_offline) {
                sprintf(format_message, SERVER_PRIVATE_ERROR_MSG, msg.toID);
                memcpy(msg.content, format_message, BUFFER_SIZE);
                bzero(send_buf, BUFFER_SIZE);
                memcpy(send_buf, &msg, sizeof(msg));
                if (send(msg.fromID, send_buf, sizeof(send_buf), 0) < 0) {
                    return -1;
                }
            }
        }
    }
    return len;
}

void Server::Start() {
    static struct epoll_event events[EPOLL_SIZE];

    Init();

    while (1) {
        int epoll_events_count = epoll_wait(epfd, events, EPOLL_SIZE, -1);
        if (epoll_events_count < 0) {
            perror("epoll_wait");
            exit(-1);
        }

        std::cout << "epoll_events_count = " << epoll_events_count << std::endl;

        for (int i=0; i<epoll_events_count; i++) {
            int sockfd = events[i].data.fd;
            // New user connection
            if (sockfd == listener) {
                struct sockaddr_in client_addr;
                socklen_t client_addr_length = sizeof(struct sockaddr_in);
                int clientfd = accept(listener, (struct sockaddr *)&client_addr, &client_addr_length);

                std::cout << "client connection from: "
                    << inet_ntoa(client_addr.sin_addr) << ":"
                    << ntohs(client_addr.sin_port) << ", clientfd = "
                    << clientfd << std::endl;

                addFd(epfd, clientfd, true);

                // server save new connect client in list
                client_list.push_back(clientfd);

                // welcome
                std::cout << "welcome msg" << std::endl;
                char msg[BUFFER_SIZE];
                bzero(msg, BUFFER_SIZE);
                sprintf(msg, SERVER_WELCOME, clientfd);
                if (send(clientfd, msg, BUFFER_SIZE, 0) < 0) {
                    perror("send");
                    exit(-1);
                }
                // handle messag receive and broad
                else {
                    if (SendBroadcastMessage(sockfd) < 0) {
                        perror("SendBroadcastMessage");
                        exit(-1);
                    }
                }
            }
        }
    }
    Close();
}
