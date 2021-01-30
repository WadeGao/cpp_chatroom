/*
 * @Author: your name
 * @Date: 1969-12-31 16:00:00
 * @LastEditTime: 2021-01-30 16:17:50
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

class Server
{
private:
    int SendBroadcastMsg(int clientfd);
    struct sockaddr_in serverAddr;
    int listener;
    int epfd;
    std::list<int> client_list;

    Database db;
    std::vector<char *> DB_IP_List;
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

public:
    Server();
    ~Server();
    void Init();
    void Start();
    void Close() const;
};

#endif