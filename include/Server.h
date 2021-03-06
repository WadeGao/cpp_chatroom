/*
 * @Author: your name
 * @Date: 1969-12-31 16:00:00
 * @LastEditTime: 2021-03-10 16:58:31
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /cpp-imsoftware/include/Server.h
 */
#ifndef SERVER_H_
#define SERVER_H_

#include "Common.h"
#include "Database.h"
#include "ThreadPool.hpp"
#include <unordered_map>
#include <utility>

#define EPOLL_SIZE 1024
//内核TCP已发送队列长度
#define BACK_LOG 1024

//MySQL服务器配置信息
//#define DATABASE_DOMAIN "wadegao.tpddns.net"
#define DATABASE_DOMAIN "wadegao.tpddns.net"
#define DATABASE_NAME "ChatRoom"
#define DATABASE_ADMIN "ServerChatRoom"

//格式化报文信息
#define SERVER_WELCOME "Welcome to join the chat room! You Nickname is %s"
#define SERVER_MSG "%s >>> %s\n"
#define CAUTION "You're the only one in the chat room!"

//状态码
enum SERVER_CHECK_CODE
{
    SERVER_FAIL_CONNECT_DB = 1,
    SERVER_GETADDRINFO_ERROR,
    UNSUPPORTED_PROTOCOL,
    SERVER_SETSOCKOPT_ERROR,
    BIND_ERROR,
    LISTEN_ERROR,
    ACCEPT_ERROR,
    SERVER_GETNAMEINFO_ERROR,
    SERVER_EPOLL_CREATE_ERROR,
    SERVER_INET_NTOP_ERROR,
    SERVER_SEND_ERROR,
    SENDBROADCASTMSG_ERROR,
    SENDLOGINSTATUS_ERROR,
    SENDPRIVATEMSG_ERROR,
    SENDONLINELIST_ERROR,
    SENDACCEPTNORMALOFFLINE,
};

class Server
{
private:
    Database db;
    ThreadPool myPool;
    std::vector<std::future<ssize_t>> ProcessingFunctionRet;
    int newUserListener{0};
    int epfd{0};

    std::list<int> client_list;
    std::unordered_map<int, std::pair<std::string, std::string>> Fd2_ID_Nickname;
    std::unordered_map<std::string, int> ID_fd;

    bool initOneServerListener(int &listener, const char *port);
    //TODO:更改fd状态时是否可以加const
    bool initAllServerListener(std::list<std::pair<int, const char *>> &Listener_Port_List);
    bool Conn2DB(const char *db_domain, const char *port, const char *db_account, const char *db_name);

    std::string GetNickName(const std::string &ClientID);     //服务端查询数据库获取账号昵称
    void AddMappingInfo(int clientfd, const std::string &ID); //删除两个映射表的对应表项以及客户端文件描述符队列
    void RemoveMappingInfo(int clientfd);                     //删除两个映射表的对应表项以及客户端文件描述符队列
    void MakeSomeoneOffline(int clientfd, bool isForced);

    ssize_t SendLoginStatus(int clientfd, const LoginStatusCodeType authVerifyStatusCode);
    ssize_t SendWelcomeMsg(int clientfd);
    ssize_t SendBroadcastMsg(const int clientfd, const char *MessageToSend);
    ssize_t SendPrivateMsg(const int fd_from, const int fd_to, const char *MessageToSend);
    ssize_t SendOnlineList(const int clientfd);
    ssize_t SendAcceptNormalOffline(const int clientfd);

    bool CheckIfAccountExistInMySQL(const std::string &ID);
    bool CheckIsAccountPasswordMatch(const std::string &ID, const std::string &Password);
    bool CheckIsDuplicatedLoggin(const std::string &ID);
    LoginStatusCodeType AccountVerification(const char *ID, const char *Password);

public:
    Server();
    ~Server() = default;

    void Start();
    void Close() const;
};
#endif
