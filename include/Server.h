/*
 * @Author: your name
 * @Date: 1969-12-31 16:00:00
 * @LastEditTime: 2021-03-05 10:44:35
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /cpp-imsoftware/include/Server.h
 */
#ifndef __SERVER_H__
#define __SERVER_H__

#include "Common.h"
#include "Database.h"
#include <unordered_map>
#include <utility>

#define EPOLL_SIZE 5000
//内核TCP已发送队列长度
#define BACK_LOG 1024

//MySQL服务器配置信息
#define DATABASE_DOMAIN "wadegao.tpddns.net"
#define DATABASE_NAME "ChatRoom"
#define DATABASE_ADMIN "ServerChatRoom"

//格式化报文信息
#define SERVER_WELCOME "\033[31mWelcome to join the chat room! You Nickname is %s\033[0m"
#define SERVER_MSG "\033[32m%s >>> %s\033[0m\a"
#define CAUTION "\033[31mYou're the only one in the chat room!\033[0m"

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
};

class Server
{
private:
    char msg[BUF_SIZE]{0};
    ssize_t SendBroadcastMsg(int clientfd);
    int listener{0};
    int epfd{0};
    std::list<int> client_list;

    Database db;
    std::unordered_map<int, std::pair<std::string, std::string>> Fd2_ID_Nickname;
    std::unordered_map<std::string, bool> If_Duplicated_Loggin;

    //服务端账号身份校核
    LoginStatusCodeType AccountVerification(const ClientIdentity &thisConnIdentity);
    //服务端查询数据库获取账号昵称
    std::string GetNickName(const std::string &ClientID);
    //如果返回值为true，代表此账号已经登录，应该拒绝提供服务
    bool IsDuplicatedLoggin(const std::string &ID);
    //删除两个映射表的对应表项以及客户端文件描述符队列
    void AddMappingInfo(int clientfd, const std::string &ID_buf);
    //删除两个映射表的对应表项以及客户端文件描述符队列
    void RemoveMappingInfo(int clientfd);

    void SendLoginStatus(int clientfd, const LoginStatusCodeType &authVerifyStatusCode);

public:
    Server();
    ~Server() = default;
    void Conn2DB(const char *db_domain, const char *port, const char *db_account, const char *db_name);
    void Start();
    void Close() const;
};

#endif
