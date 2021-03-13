/*
 * @Author: your name
 * @Date: 2021-03-09 16:26:34
 * @LastEditTime: 2021-03-12 22:06:24
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /cpp-imsoftware/src/Common/Common.cpp
 */
#include "Common.h"

void addfd(int epollfd, int fd, bool enable_et)
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

void fdAutoCloser(int fd)
{
    auto exitJob_sock = [](int status, void *fd) -> void { close(*((int *)fd)); };
    on_exit(exitJob_sock, &fd);
}

bool Domain2IP(const char *domain, const int SOCK_TYPE, char *IP, const size_t len)
{
    if (len < 128)
        return false;

    addrinfo *ret, *cur, hints{};
    bzero(&hints, sizeof(addrinfo));
    hints.ai_socktype = SOCK_TYPE;
    hints.ai_family = AF_UNSPEC; /* Allow IPv4 or IPv6 */
    auto err = getaddrinfo(domain, nullptr, &hints, &ret);
    if (err != 0)
    {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(err));
        return false;
    }

    for (cur = ret; cur; cur = cur->ai_next)
    {
        bzero(IP, len);
        if (cur->ai_family == AF_INET6)
        {
            auto sockAddr_IPv6 = reinterpret_cast<sockaddr_in6 *>(cur->ai_addr);
            if (inet_ntop(cur->ai_family, &sockAddr_IPv6->sin6_addr, IP, len))
                break;
        }
        else if (cur->ai_family == AF_INET)
        {
            auto sockAddr_IPv4 = reinterpret_cast<sockaddr_in *>(cur->ai_addr);
            if (inet_ntop(cur->ai_family, &sockAddr_IPv4->sin_addr, IP, len))
                break;
        }
    }
    if (!cur)
        return false;
    freeaddrinfo(ret);
    return true;
}

std::string getTime()
{
    time_t timep;
    time(&timep);
    char ret[24];
    strftime(ret, sizeof(ret), "%Y-%m-%d %H:%M:%S", localtime(&timep));
    return std::string(ret);
}

bool isFolderOrFileExist(const char *path) { return access(path, F_OK) < 0 ? false : true; }
