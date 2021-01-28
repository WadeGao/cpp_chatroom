#include "Client.h"
#include <iostream>
#include <utility>

Client::Client(std::string id, std::string pwd) : ClientID(std::move(id)), ClientPwd(std::move(pwd)), sock(0), pid(0), isClientWork(true), epfd(0)
{
    auto host = gethostbyname(SERVER_DOMAIN);
    if (!host)
    {
        fprintf(stderr, "Can't resolve domain %s\n", SERVER_DOMAIN);
        exit(EXIT_FAILURE);
    }
    serverAddr.sin_family = host->h_addrtype;
    serverAddr.sin_addr.s_addr = inet_addr(inet_ntoa(*(struct in_addr *)host->h_addr_list[0]));
    serverAddr.sin_port = htons(SERVER_PORT);
}

Client::~Client() = default;

void Client::Connect()
{

    //建立转发服务器连接
    //fprintf(stdout, "Connect to Server %s:%u\n", SERVER_IP, SERVER_PORT);
    if ((sock = socket(serverAddr.sin_family, SOCK_STREAM, 0)) < 0)
    {
        fprintf(stderr, "socket() error\n");
        exit(EXIT_FAILURE);
    }

    if (connect(sock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        fprintf(stderr, "Can't connect to server\n");
        exit(EXIT_FAILURE);
    }

    if (pipe(pipe_fd) < 0)
    {
        fprintf(stderr, "pipe() error");
        exit(EXIT_FAILURE);
    }

    if ((epfd = epoll_create(EPOLL_SIZE)) < 0)
    {
        fprintf(stderr, "epoll_create() error\n");
        exit(EXIT_FAILURE);
    }

    addfd(epfd, sock, true);
    addfd(epfd, pipe_fd[0], true);

    //告知用户身份
    sprintf(msg, "[%lu]%s%s", this->ClientID.size(), this->ClientID.c_str(), this->ClientPwd.c_str());
    send(sock, msg, BUF_SIZE, 0);
}

void Client::Start()
{
    static struct epoll_event events[2];
    Connect();

    if ((pid = fork()) < 0)
    {
        fprintf(stderr, "fork() error\n");
        close(sock);
        exit(EXIT_FAILURE);
    }
    else if (!pid)
    {
        close(pipe_fd[0]);
        fprintf(stdout, "\033[31mPlease input 'LOGOUT' to exit the chat room\n\033[0m");
        while (isClientWork)
        {
            bzero(msg, BUF_SIZE);
            fgets(msg, BUF_SIZE, stdin);
            if (strncasecmp(msg, LOGOUT, strlen(LOGOUT)) == 0)
                isClientWork = false;
            else
            {
                if (write(pipe_fd[1], msg, strlen(msg) - 1) < 0)
                {
                    fprintf(stderr, "child process write error\n");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    else
    {
        close(pipe_fd[1]);
        while (isClientWork)
        {
            auto epoll_event_count = epoll_wait(epfd, events, 2, -1);
            for (int i = 0; i < epoll_event_count; i++)
            {
                bzero(msg, BUF_SIZE);
                if (events[i].data.fd == sock) //服务端发来消息
                {
                    int ret = recv(sock, msg, BUF_SIZE, 0);
                    if (!ret)
                    {
                        fprintf(stderr, "Server closed.\n");
                        close(sock);
                        isClientWork = false;
                    }
                    else
                        fprintf(stdout, "%s\n", msg);
                }
                else //子进程写入事件发生，父进程处理并发送服务端
                {
                    int ret = read(events[i].data.fd, msg, BUF_SIZE);
                    !ret ? isClientWork = false : send(sock, msg, BUF_SIZE, 0);
                }
            }
        }
    }
    Close();
}

void Client::Close()
{
    if (pid)
    {
        close(pipe_fd[0]);
        close(sock);
    }
    else
        close(pipe_fd[1]);
}
