#include "Client.h"
#include <iostream>

Client::Client(const std::string &id, const std::string &pwd) : ClientID(id), password(pwd), sock(0), pid(0), isClientWork(true), epfd(0)
{
    serverAddr.sin_family = PF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    serverAddr.sin_port = htons(SERVER_PORT);
}

Client::~Client() {}

bool Client::CheckIfAccountExist(const std::string &account)
{
    const std::string sql = "SELECT ClientID FROM ChatRoom WHERE ClientID = '" + account + "';";
    return (this->db.ReadMySQL(sql).size() == 1);
}

bool Client::CheckPassword(const std::string &account, const std::string &pwd)
{
    const std::string sql = "SELECT ClientPassword FROM ChatRoom WHERE ClientID = '" + account + "';";
    auto ret = this->db.ReadMySQL(sql);
    return (ret.size() == 1 && ret.at(0).at(0) == pwd);
}

void Client::CheckDatabaseJobBeforeServe()
{
    if (!CheckIfAccountExist(this->ClientID))
    {
        fprintf(stderr, "Account %s not exists.\n", this->ClientID.c_str());
        exit(CLIENTID_NOT_EXIST);
    }

    if (!CheckPassword(this->ClientID, this->password))
    {
        fprintf(stderr, "Invalid password!\n");
        exit(WRONG_CLIENT_PASSWORD);
    }
}

std::string Client::GetNickName()
{
    auto sql = "SELECT ClientNickname FROM ChatRoom WHERE ClientID = '" + this->ClientID + "';";
    auto __nickname = this->db.ReadMySQL(sql).at(0).at(0);
    return __nickname;
}

void Client::Connect()
{
    //检查数据库连接性
    //fprintf(stdout, "Connect to Database %s@%s:%s\n", DATABASE_ADMIN, DATABASE_IP, DATABASE_NAME);
    if (!this->db.ConnectMySQL(DATABASE_IP, DATABASE_ADMIN, DATABASE_NAME, true, DATABASE_PWD))
    {
        fprintf(stderr, "Failed to Connect Database.\n");
        exit(FAIL_CONNECT_DB);
    }
    this->CheckDatabaseJobBeforeServe();

    //建立转发服务器连接
    //fprintf(stdout, "Connect to Server %s:%u\n", SERVER_IP, SERVER_PORT);

    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        fprintf(stderr, "socket() error\n");
        exit(EXIT_FAILURE);
    }

    if (connect(sock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        fprintf(stderr, "Can't connect to server error\n");
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

    //获取用户昵称
    this->NickName = this->GetNickName();
    //fprintf(stdout, "Your nickname is %s.\n", this->NickName.c_str());

    //告知用户身份
    sprintf(msg, "%s", this->ClientID.c_str());
    send(sock, msg, BUF_SIZE, 0);
}

void Client::Start()
{
    static struct epoll_event events[2];

    Connect();

    pid = fork();
    if (pid < 0)
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