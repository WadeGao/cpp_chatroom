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