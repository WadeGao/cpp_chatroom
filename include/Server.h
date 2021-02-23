/*
 * @Author: your name
 * @Date: 1969-12-31 16:00:00
 * @LastEditTime: 2021-02-23 12:35:17
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

#define BACK_LOG 1024
class Server
{
private:
    char msg[BUF_SIZE]{0};
    ssize_t SendBroadcastMsg(int clientfd);
    int listener;
    int epfd;
    std::list<int> client_list;

    Database db;
    std::unordered_map<int, std::pair<std::string, std::string>> Fd2_ID_Nickname;
    std::unordered_map<std::string, bool> If_Duplicated_Loggin;

    //服务端账号身份校核
    size_t AccountVerification(const std::string &ClientID, const std::string &ClientPwd);
    //服务端查询数据库获取账号昵称
    std::string GetNickName(const std::string &ClientID);
    //如果返回值为true，代表此账号已经登录，应该拒绝提供服务
    bool IsDuplicatedLoggin(const std::string &ID);
    //删除两个映射表的对应表项以及客户端文件描述符队列
    void AddMappingInfo(int clientfd, const std::string &ID_buf);
    //删除两个映射表的对应表项以及客户端文件描述符队列
    void RemoveMappingInfo(int clientfd);

    std::vector<std::string> ShakeHandMsgParser(const std::string &msg_buf);

    void SendLoginStatus(int clientfd, const size_t authVerifyStatusCode);

public:
    Server();
    ~Server() = default;
    void Conn2DB(const char *db_domain, const char *port, const char *db_account, const char *db_name);
    void Start();
    void Close() const;
};

#endif