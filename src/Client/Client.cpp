#include "Client.h"
#include <iostream>
#include <sys/wait.h>

static void handlerSIGCHLD(int signo)
{
    pid_t PID;
    int stat;
    while ((PID = waitpid(-1, &stat, WNOHANG)) > 0)
        fprintf(stdout, "child %d terminated\n", PID);
}

Client::Client(const char *id, const char *pwd)
{
    memcpy(this->myIdentity.ID, id, strlen(id));
    memcpy(this->myIdentity.Password, pwd, strlen(pwd));

    addrinfo hints{}, *ret, *cur;
    bzero(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC; /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_ALL;
    hints.ai_protocol = 0;
    auto err = getaddrinfo(SERVER_DOMAIN, SERVER_PORT, &hints, &ret);
    if (err != 0)
    {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(err));
        exit(CLIENT_GETADDRINFO_ERROR);
    }

    for (cur = ret; cur; cur = cur->ai_next)
    {
        if ((this->sock = socket(cur->ai_family, cur->ai_socktype, cur->ai_protocol)) < 0)
            continue;
        if (connect(this->sock, cur->ai_addr, cur->ai_addrlen) == 0)
            break;
        close(this->sock);
    }
    if (!cur)
    {
        fprintf(stderr, "Can't connect to server\n");
        exit(CONNECT_ERROR);
    }

    freeaddrinfo(ret);

    if (pipe(pipe_fd) < 0)
    {
        fprintf(stderr, "pipe() error");
        exit(CLIENT_PIPE_ERROR);
    }

    if ((epfd = epoll_create(2)) < 0)
    {
        fprintf(stderr, "epoll_create() error\n");
        exit(CLIENT_EPOLLCREATE_ERROR);
    }

    fdAutoCloser(epfd);

    addfd(this->epfd, this->sock, true);
    addfd(this->epfd, pipe_fd[0], true);
}

void Client::Connect()
{
    //告知用户身份
    this->TellMyIdentity();
    this->RecvLoginStatus();
}

//向服务器发送输入的ID和密码
void Client::TellMyIdentity()
{
    if (send(this->sock, reinterpret_cast<const void *>(&this->myIdentity), sizeof(ClientIdentity), 0) < 0)
    {
        fprintf(stderr, "Send authentication error\n");
        this->Close();
        exit(CLIENT_SEND_ERROR);
    }
}

void Client::RecvLoginStatus()
{
    bzero(&this->msg, BUF_SIZE);

    auto old_fd_status = fcntl(this->sock, F_GETFD, 0);
    fcntl(this->sock, F_SETFL, fcntl(this->sock, F_GETFD, 0) & ~O_NONBLOCK);
    LoginStatusCodeType status;
    auto len = recv(this->sock, reinterpret_cast<void *>(&status), BUF_SIZE, 0);
    fcntl(this->sock, F_SETFL, old_fd_status);

    if (len < 0)
    {
        fprintf(stderr, "Error occurs, %s\n", strerror(errno));
        this->Close();
        exit(CLIENT_RECV_ERROR);
    }

    switch (status)
    {
    case CLIENT_ID_NOT_EXIST:
        fprintf(stderr, "Account not exists, please check your account\n");
        this->Close();
        exit(CLIENT_ID_NOT_EXIST);
    case WRONG_CLIENT_PASSWORD:
        fprintf(stderr, "Account's password error, please check your password\n");
        this->Close();
        exit(WRONG_CLIENT_PASSWORD);
    case DUPLICATED_LOGIN:
        fprintf(stderr, "Account has been online!\n");
        this->Close();
        exit(DUPLICATED_LOGIN);
    default:
        break;
    }
}

void Client::Start()
{
    static struct epoll_event events[2];
    this->Connect();
    signal(SIGCHLD, handlerSIGCHLD);
    fflush(nullptr);

    if ((this->pid = fork()) < 0)
    {
        fprintf(stderr, "fork() error\n");
        exit(CLIENT_FORK_ERROR);
    }
    else if (!this->pid)
    {
        close(this->pipe_fd[0]);
        fprintf(stdout, "\033[31mPlease input 'LOGOUT' to exit the chat room\n\033[0m");
        while (this->isClientWork)
        {
            bzero(this->msg, BUF_SIZE);
            fgets(this->msg, BUF_SIZE, stdin);
            if (!strncasecmp(this->msg, LOGOUT, strlen(LOGOUT)))
                this->isClientWork = false;
            else
            {
                if (write(this->pipe_fd[1], this->msg, strlen(this->msg) - 1) < 0)
                    exit(CLIENT_WRITE_ERROR);
            }
        }
    }
    else
    {
        close(this->pipe_fd[1]);
        while (this->isClientWork)
        {
            auto epoll_event_count = epoll_wait(this->epfd, events, 2, -1);
            for (int i = 0; i < epoll_event_count; i++)
            {
                bzero(&this->msg, BUF_SIZE);
                if (events[i].data.fd == this->sock) //服务端发来消息
                {
                    if (!recv(this->sock, this->msg, BUF_SIZE, 0))
                    {
                        fprintf(stderr, "Server closed.\n");
                        this->isClientWork = false;
                    }
                    else
                        fprintf(stdout, "%s\n", this->msg);
                }
                else //子进程写入事件发生，父进程处理并发送服务端
                    !read(events[i].data.fd, this->msg, BUF_SIZE) ? this->isClientWork = false : send(this->sock, this->msg, strlen(this->msg), 0);
            }
        }
    }
    this->Close();
}

void Client::Close() { close(this->pipe_fd[!this->pid ? 1 : 0]); }
