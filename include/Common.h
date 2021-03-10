/*
 * @Author: your name
 * @Date: 1969-12-31 16:00:00
 * @LastEditTime: 2021-03-10 21:36:44
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /cpp-imsoftware/include/Common.h
 */
#ifndef COMMON_H_
#define COMMON_H_

#include <arpa/inet.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <iostream>
#include <list>
#include <netdb.h>
#include <netinet/in.h>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

//转发服务器配置信息
#define SERVER_DOMAIN "wadegao.tpddns.net"
#define SERVER_NEW_USER_PORT "8888"

const size_t BUF_SIZE = 2048;

//身份格式限制
#define MAX_ACCOUNT_LEN 32
#define MAX_PASSWORD_LEN 32
#define MAX_NICKNAME_LEN 50

//身份识别状态码
#define CLIENT_CHECK_SUCCESS 200
#define CLIENT_ID_NOT_EXIST 501
#define WRONG_CLIENT_PASSWORD 502
#define DUPLICATED_LOGIN 503

//消息操作码
#define CLIENT_LOGIN_MSG 600
#define SERVER_LOGIN_MSG 605

#define PRIVATE_MSG 610
#define GROUP_MSG 615

#define REQUEST_ONLINE_LIST 700
#define REPLY_ONLINE_LIST 705

#define REQUEST_NORMAL_OFFLINE 800
#define REPLY_NORMAL_OFFLINE 805

#define FORCE_OFFLINE 900

using LoginStatusCodeType = uint16_t;

void addfd(int epollfd, int fd, bool enable_et);

void fdAutoCloser(int fd);

bool Domain2IP(const char *domain, const int SOCK_TYPE, char *IP, const size_t len);

std::string getTime();

class MessageType
{
public:
    uint16_t OperCode;
};

class ClientLoginMessageType : public MessageType
{
public:
    char ID[MAX_ACCOUNT_LEN]{0};
    char Password[MAX_PASSWORD_LEN]{0};
};

class ServerLoginCodeMessageType : public MessageType
{
public:
    uint16_t CheckCode;
};

class ChatMessageType : public MessageType
{
public:
    char Msg[BUF_SIZE]{0};
    char Whom[MAX_NICKNAME_LEN]{0};
};

class OnlineListMessageType : public MessageType
{
public:
    uint32_t OnLineNum;
    char List[BUF_SIZE]{0};
};

const auto maxBufToMalloc = sizeof(ChatMessageType);

using LogoutMessageType = MessageType;
using OnlineListRequestType = MessageType;

#endif
