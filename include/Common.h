/*
 * @Author: your name
 * @Date: 1969-12-31 16:00:00
 * @LastEditTime: 2021-01-30 20:45:59
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /cpp-imsoftware/include/Common.h
 */
#ifndef __COMMON_H__
#define __COMMON_H__

#include <arpa/inet.h>
#include <cerrno>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <list>
#include <netinet/in.h>
#include <string>
#include <sys/epoll.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

//状态码
enum CHECK_CODE
{
    CHECK_SUCCESS = 1,
    FAIL_CONNECT_DB,
    CLIENTID_NOT_EXIST,
    WRONG_CLIENT_PASSWORD,
    DUPLICATED_LOGIN
};

//转发服务器配置信息
#define SERVER_DOMAIN "wadegao.tpddns.net"
#define SERVER_PORT 8888
#define EPOLL_SIZE 5000
#define BUF_SIZE 2048

//MySQL服务器配置信息
#define DATABASE_DOMAIN "wadegao.tpddns.net"
#define DATABASE_NAME "ChatRoom"
#define DATABASE_ADMIN "root"
#define DATABASE_PWD "zsy19990607"

//格式化报文信息
#define SERVER_WELCOME "\033[31mWelcome to join the chat room! You Nickname is %s\033[0m"
#define SERVER_MSG "\033[32m%s >>> %s\033[0m\a"
#define LOGOUT "LOGOUT"
#define CAUTION "\033[31mYou're the only one in the chat room!\033[0m"
#define DENY_SERVE "Login Code:%c"

static void addfd(int epollfd, int fd, bool enable_et)
{
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN;
    if (enable_et)
        ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
    //fprintf(stdout, "fd added to epoll.\n");
}

#endif
