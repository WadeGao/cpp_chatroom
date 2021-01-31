#include <iostream>
#include <utility>
#include "Client.h"

Client::Client(std::string id, std::string pwd) : ClientID(std::move(id)), ClientPwd(std::move(pwd)), sock(0), pid(0), isClientWork(true), epfd(0)
{
    auto host = gethostbyname(SERVER_DOMAIN);
    if (!host)
    {
        fprintf(stderr, "Can't resolve domain %s\n", SERVER_DOMAIN);
        exit(EXIT_FAILURE);
    }
    serverAddr.sin_family = host->h_addrtype;

    for (size_t i = 0; host->h_addr_list[i]; i++)
        this->ServerIP_List.push_back(inet_ntoa(*(struct in_addr *)host->h_addr_list[i]));
    //TODO:转发服务器负载均衡
    //serverAddr.sin_addr.s_addr = inet_addr(inet_ntoa(*(struct in_addr *)host->h_addr_list[0]));
    serverAddr.sin_addr.s_addr = inet_addr(this->ServerIP_List.at(LoadBalancer(this->ServerIP_List.size())));
    serverAddr.sin_port = htons(SERVER_PORT);
}

Client::~Client() = default;

void Client::Connect()
{

    if ((sock = socket(serverAddr.sin_family, SOCK_STREAM, 0)) < 0)
    {
        fprintf(stderr, "socket() error\n");
        exit(EXIT_FAILURE);
    }

    fdAutoCloser(sock);

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

    fdAutoCloser(epfd);

    addfd(epfd, sock, true);
    addfd(epfd, pipe_fd[0], true);

    //告知用户身份
    this->TellMyIdentity();
    this->RecvLoginStatus();
}

//向服务器发送输入的ID和密码
void Client::TellMyIdentity()
{
    bzero(this->msg, BUF_SIZE);
    sprintf(this->msg, "[%lu]%s%s", this->ClientID.size(), this->ClientID.c_str(), this->ClientPwd.c_str());

    if (send(this->sock, this->msg, strlen(this->msg), 0) < 0)
    {
        fprintf(stderr, "send() auth info error\n");
        this->Close();
        exit(EXIT_FAILURE);
    }
}

void Client::RecvLoginStatus()
{
    bzero(this->msg, BUF_SIZE);
    auto old_fd_status = fcntl(this->sock, F_GETFD, 0);
    fcntl(this->sock, F_SETFL, fcntl(this->sock, F_GETFD, 0) & ~O_NONBLOCK);
    auto len = recv(this->sock, this->msg, BUF_SIZE, 0);
    fcntl(this->sock, F_SETFL, old_fd_status);

    if (len < 0)
    {
        fprintf(stderr, "Error occurs, %s\n", strerror(errno));
        this->Close();
        exit(EXIT_FAILURE);
    }
    auto AuthStatus = this->LoginAuthInfoParser(this->msg);

    switch (AuthStatus)
    {
    case CLIENTID_NOT_EXIST:
        fprintf(stderr, "Account not exists, please check your account\n");
        Close();
        exit(CLIENTID_NOT_EXIST);
        break;
    case WRONG_CLIENT_PASSWORD:
        fprintf(stderr, "Account's password error, please check your password\n");
        Close();
        exit(WRONG_CLIENT_PASSWORD);
        break;
    case DUPLICATED_LOGIN:
        fprintf(stderr, "Account has been online!\n");
        Close();
        exit(DUPLICATED_LOGIN);
        break;
    default:
        break;
    };
}

void Client::Start()
{
    static struct epoll_event events[2];
    Connect();

    if ((pid = fork()) < 0)
    {
        fprintf(stderr, "fork() error\n");
        //close(sock);
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
            if (!strncasecmp(msg, LOGOUT, strlen(LOGOUT)))
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
                    if (!recv(sock, msg, BUF_SIZE, 0))
                    {
                        fprintf(stderr, "Server closed.\n");
                        isClientWork = false;
                    }
                    else
                        fprintf(stdout, "%s\n", msg);
                }
                else //子进程写入事件发生，父进程处理并发送服务端
                    !read(events[i].data.fd, msg, BUF_SIZE) ? isClientWork = false : send(sock, msg, strlen(msg), 0);
            }
        }
    }
    Close();
}

void Client::Close() { this->pid ? close(pipe_fd[0]) : close(pipe_fd[1]); }

size_t Client::LoginAuthInfoParser(const std::string &str)
{
    bool flag = ((str.size() == 12) && (str.substr(0, 10) == "Login Code"));
    if (!flag)
    {
        fprintf(stderr, "Error package format\n");
        this->Close();
    }
    return size_t(str.at(11));
}