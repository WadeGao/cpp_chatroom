/*
 * @Author: your name
 * @Date: 2020-12-20 21:42:24
 * @LastEditTime: 2020-12-25 10:47:48
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /IM_Software/Client.h
 */
#ifndef __CLIENT_H__
#define __CLIENT_H__

#include "Common.h"

class Client
{
private:
    int sock;
    int pid;
    int epfd;
    int pipe_fd[2]{};
    bool isClientWork;
    char msg[BUF_SIZE]{};
    struct sockaddr_in serverAddr{};

    std::string ClientID;
    std::string ClientPwd;

    void Connect();

public:
    Client(std::string id, std::string pwd);
    ~Client();
    void Start();

    void Close();
};

#endif