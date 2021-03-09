/*
 * @Author: your name
 * @Date: 1969-12-31 16:00:00
 * @LastEditTime: 2021-03-09 16:24:22
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /cpp-imsoftware/include/Client.h
 */
#ifndef __CLIENT_H__
#define __CLIENT_H__

#include "Common.h"
//状态码
enum CLIENT_CHECK_CODE
{
    SERVER_CRASHED = 1,
    CLIENT_GETADDRINFO_ERROR,
    CONNECT_ERROR,
    CLIENT_PIPE_ERROR,
    CLIENT_EPOLLCREATE_ERROR,
    CLIENT_SEND_ERROR,
    CLIENT_RECV_ERROR,
    CLIENT_FORK_ERROR,
    CLIENT_WRITE_ERROR,
};

class Client
{
private:
    char myID[MAX_ACCOUNT_LEN]{0};
    char myPassword[MAX_PASSWORD_LEN]{0};
    char sendBuf[maxBufToMalloc]{0};
    char recvBuf[maxBufToMalloc]{0};

    int sock{0};
    pid_t pid{0};
    int epfd{0};
    int pipe_fd[2]{};
    bool isClientWork{true};

    void Connect();
    void TellMyIdentity(); //向服务器发送输入的ID和密码
    void RecvLoginStatus();

public:
    Client(const char *id, const char *pwd);
    ~Client();
    void Start();
};

#endif
