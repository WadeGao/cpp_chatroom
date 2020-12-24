/*
 * @Author: your name
 * @Date: 2020-12-20 21:00:25
 * @LastEditTime: 2020-12-23 21:59:09
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /IM_Software/Server.h
 */
#ifndef __SERVER_H__
#define __SERVER_H__

#include "Common.h"
#include <unordered_map>
#include <utility>

class Server
{
public:
    Server();
    ~Server();
    void Init();
    void Start();
    void Close();

private:
    int SendBroadcastMsg(const int clientfd);
    struct sockaddr_in serverAddr;
    int listener;
    int epfd;
    std::list<int> client_list;

    Database db;
    std::unordered_map<int, std::pair<std::string, std::string>> Fd2_ID_Nickname;
    std::unordered_map<std::string, bool> If_Duplicated_Loggin;

    //如果返回值为true，代表此账号已经登录，应该拒绝提供服务
    bool IsDuplicatedLoggin(const std::string &ID);
    //删除两个映射表的对应表项以及客户端文件描述符队列
    void AddMappingInfo(const int clientfd, const std::string &ID_buf);
    //删除两个映射表的对应表项以及客户端文件描述符队列
    void RemoveMappingInfo(const int clientfd);
};

#endif