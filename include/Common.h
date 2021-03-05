/*
 * @Author: your name
 * @Date: 1969-12-31 16:00:00
 * @LastEditTime: 2021-03-05 16:12:08
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /cpp-imsoftware/include/Common.h
 */
#ifndef __COMMON_H__
#define __COMMON_H__

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
#define SERVER_DOMAIN "127.0.0.1"
#define SERVER_PORT "8888"

#define BUF_SIZE 2048

//身份格式限制
#define MAX_ACCOUNT_LEN 32
#define MAX_PASSWORD_LEN 32

//身份识别状态码
#define CLIENT_CHECK_SUCCESS 200
#define CLIENT_ID_NOT_EXIST 501
#define WRONG_CLIENT_PASSWORD 502
#define DUPLICATED_LOGIN 503

//消息操作码
#define IDENTITY_MSG 600
#define PRIVATE_MSG 601
#define GROUP_MSG 610
#define REQUEST_ONLINE_LIST 700
#define NORMAL_OFFLINE 801
#define FORCE_OFFLINE 802

using LoginStatusCodeType = uint16_t;

typedef struct
{
    char ID[MAX_ACCOUNT_LEN]{0};
    char Password[MAX_PASSWORD_LEN]{0};
} __attribute__((packed)) ClientIdentityType;

typedef struct
{
    uint32_t OperCode;
    char msg[BUF_SIZE]{0};
} __attribute__((packed)) MessageType;

static void addfd(int epollfd, int fd, bool enable_et)
{
    struct epoll_event ev
    {
    };
    ev.data.fd = fd;
    ev.events = EPOLLIN;
    if (enable_et)
        ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}

static void fdAutoCloser(int fd)
{
    auto exitJob_sock = [](int status, void *fd) -> void { close(*((int *)fd)); };
    on_exit(exitJob_sock, &fd);
}

#endif
