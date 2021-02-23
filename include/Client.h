/*
 * @Author: your name
 * @Date: 1969-12-31 16:00:00
 * @LastEditTime: 2021-02-23 20:02:28
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
    CLIENTID_NOT_EXIST = 1,
    WRONG_CLIENT_PASSWORD,
    DUPLICATED_LOGIN,
    SERVER_CRASHED,
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
    int sock{0};
    pid_t pid{0};
    int epfd{0};
    int pipe_fd[2]{};
    bool isClientWork{true};
    char msg[BUF_SIZE]{'\0'};
    std::string ClientID;
    std::string ClientPwd;

    void Connect();

    void TellMyIdentity(); //向服务器发送输入的ID和密码

    size_t LoginAuthInfoParser(const std::string &str);

    void RecvLoginStatus();

    void Close();

    //void handlerSIGCHLD(int signo);

public:
    Client(const std::string &id, const std::string &pwd);
    ~Client() = default;
    void Start();
};

#endif