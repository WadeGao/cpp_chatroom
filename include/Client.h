/*
 * @Author: your name
 * @Date: 1969-12-31 16:00:00
 * @LastEditTime: 2021-01-28 16:56:08
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /cpp-imsoftware/include/Client.h
 */
#ifndef __CLIENT_H__
#define __CLIENT_H__

#include "Common.h"

class Client
{
private:
    int sock;
    pid_t pid;
    int epfd;
    int pipe_fd[2]{};
    bool isClientWork;
    char msg[BUF_SIZE]{};
    struct sockaddr_in serverAddr;

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