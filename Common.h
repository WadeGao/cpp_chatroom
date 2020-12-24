/*
 * @Author: your name
 * @Date: 2020-12-20 20:50:50
 * @LastEditTime: 2020-12-23 21:58:35
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /IM_Software/Common.h
 */
#ifndef __COMMON_H__
#define __COMMON_H__

#include "Database.h"
#include <arpa/inet.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <list>
#include <netinet/in.h>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8888
#define EPOLL_SIZE 5000
#define BUF_SIZE 2048
#define SERVER_WELCOME "\033[31mWelcome to join the chat room! You Nickname is %s\033[0m"
#define SERVER_MSG "\033[32m%s >>> %s\033[0m\a"
#define LOGOUT "LOGOUT"
#define CAUTION "\033[31mYou're the only one in the chat room!\033[0m"

#define DATABASE_IP "127.0.0.1"
#define DATABASE_NAME "ChatRoom"
#define DATABASE_ADMIN "root"
#define DATABASE_PWD "140603"

#define FAIL_CONNECT_DB 1
#define CLIENTID_NOT_EXIST 2
#define WRONG_CLIENT_PASSWORD 3

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