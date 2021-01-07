#include "Server.h"
#include <iostream>

Server::Server() : listener(0), epfd(0)
{
    serverAddr.sin_family = PF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
}

Server::~Server() {}

void Server::Init()
{
    fprintf(stdout, "Init Server...\n");

    if ((listener = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        fprintf(stderr, "listener error\n");
        exit(EXIT_FAILURE);
    }

    if (bind(listener, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        fprintf(stderr, "bind error\n");
        exit(EXIT_FAILURE);
    }

    if (listen(listener, 5) < 0)
    {
        fprintf(stderr, "listen error");
        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "Start to listen: %s:%u\n", SERVER_IP, SERVER_PORT);

    if ((epfd = epoll_create(EPOLL_SIZE)) < 0)
    {
        fprintf(stderr, "epoll_create() error\n");
        exit(EXIT_FAILURE);
    }

    addfd(epfd, listener, true);

    //连接到数据库
    if (!this->db.ConnectMySQL(DATABASE_IP, DATABASE_ADMIN, DATABASE_NAME, true, DATABASE_PWD))
    {
        fprintf(stderr, "Failed to Connect Database.\n");
        exit(FAIL_CONNECT_DB);
    }
}

void Server::Start()
{
    static struct epoll_event events[EPOLL_SIZE];
    Init();
    while (true)
    {
        auto epoll_events_count = epoll_wait(epfd, events, EPOLL_SIZE, -1);
        if (epoll_events_count < 0)
        {
            fprintf(stderr, "epoll fail\n");
            break;
        }
        fprintf(stdout, "epoll_events_count = %d\n", epoll_events_count);

        for (int i = 0; i < epoll_events_count; i++)
        {
            auto sockfd = events[i].data.fd;
            if (sockfd == listener)
            {
                struct sockaddr_in client_address;
                socklen_t client_addrLength = sizeof(struct sockaddr_in);
                auto clientfd = accept(listener, (struct sockaddr *)&client_address, &client_addrLength);

                //接收首次通信时由客户端告知的连接用户身份信息
                char ID_Pwd_buf[BUF_SIZE] = {0};
                auto len = recv(clientfd, ID_Pwd_buf, BUF_SIZE, 0);
                if (!len)
                    continue;

                auto vectorID_Pwd = this->ShakeHandMsgParser(ID_Pwd_buf);

                if (!this->AccountVerification(vectorID_Pwd.at(0), vectorID_Pwd.at(1)))
                {
                    close(clientfd);
                    fprintf(stderr, "\033[31mAccount Verification Failed! Account not exists or wrong password!\n\033[0m");
                    continue;
                    //至此账号已经通过核查，账号真实存在，提供的口令也正确
                }

                //检查是否同一账号登录多次
                if (this->IsDuplicatedLoggin(vectorID_Pwd.at(0)))
                {
                    close(clientfd);
                    fprintf(stderr, "\033[31mDenied a duplicated user's connection\n\033[0m");
                    continue;
                    //至此账号单例检查已通过
                }

                // 查数据库完善信息，并添加信息到系统文件描述表和映射表
                this->AddMappingInfo(clientfd, vectorID_Pwd.at(0));

                addfd(epfd, clientfd, true);
                fprintf(stdout, "\033[31mClient connection from %s:%u, clientfd = %d\n\033[0m", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port), clientfd);
                fprintf(stdout, "Add new clientfd = %d to epoll. Now there are %lu client(s) int the chat room\n", clientfd, client_list.size());
                fprintf(stdout, "Welcome Message\n");

                char msg[BUF_SIZE] = {0};
                sprintf(msg, SERVER_WELCOME, this->Fd2_ID_Nickname.find(clientfd)->second.second.c_str());
                if (send(clientfd, msg, BUF_SIZE, 0) < 0)
                {
                    fprintf(stderr, "send() welcome msg error\n");
                    Close();
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                if (SendBroadcastMsg(sockfd) < 0)
                {
                    fprintf(stderr, "SendBroadcastMsg() error\n");
                    Close();
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    Close();
}

int Server::SendBroadcastMsg(const int clientfd)
{
    char buf[BUF_SIZE] = {0}, msg[BUF_SIZE] = {0};
    fprintf(stdout, "read from client %d\n", clientfd);

    int len = recv(clientfd, buf, BUF_SIZE, 0);

    if (!len)
    {
        close(clientfd);

        this->RemoveMappingInfo(clientfd);

        fprintf(stdout, "\033[31mClient %d closed.\n\033[0m", clientfd);
        fprintf(stdout, "Now there are %lu client(s) in the chat room.\n", client_list.size());
    }
    else
    {
        if (client_list.size() == 1)
        {
            send(clientfd, CAUTION, strlen(CAUTION), 0);
            return len;
        }
        sprintf(msg, SERVER_MSG, this->Fd2_ID_Nickname.find(clientfd)->second.second.c_str(), buf);
        for (const auto &iter : client_list)
        {
            if (iter != clientfd)
                if (send(iter, msg, BUF_SIZE, 0) < 0)
                    return -1;
        }
    }
    return len;
}

void Server::Close()
{
    close(listener);
    close(epfd);
}

bool Server::IsDuplicatedLoggin(const std::string &ID)
{
    auto iter = this->If_Duplicated_Loggin.find(ID);
    return (iter == this->If_Duplicated_Loggin.end()) ? false : iter->second;
}

void Server::AddMappingInfo(const int clientfd, const std::string &ID_buf)
{
    //根据账号ID查数据库得到账号昵称
    auto nickname = this->GetNickName(ID_buf);
    //写入文件描述符与账号信息的映射表
    this->Fd2_ID_Nickname.insert(std::make_pair(clientfd, std::make_pair(ID_buf, nickname)));
    //写入在线账号ID表
    this->If_Duplicated_Loggin.insert(std::make_pair(ID_buf, true));
    //服务端用list保存用户连接
    this->client_list.push_back(clientfd);
}

void Server::RemoveMappingInfo(const int clientfd)
{
    auto closedAccount = this->Fd2_ID_Nickname.find(clientfd)->second.first;
    //删除文件描述符->账号信息映射表
    this->Fd2_ID_Nickname.erase(this->Fd2_ID_Nickname.find(clientfd));
    //删除在线账号列表表项
    this->If_Duplicated_Loggin.erase(this->If_Duplicated_Loggin.find(closedAccount));
    //将该账号对应的文件描述符从在线文件描述符集中删除
    this->client_list.remove(clientfd);
}

bool Server::AccountVerification(const std::string &Account, const std::string &Pwd)
{
    auto CheckIfAccountExist = [&Account, this]() -> bool {
        const std::string sql = "SELECT ClientID FROM ChatRoom WHERE ClientID = '" + Account + "';";
        return (this->db.ReadMySQL(sql).size() == 1);
    };
    auto CheckPassword = [&Account, &Pwd, this]() -> bool {
        const std::string sql = "SELECT ClientPassword FROM ChatRoom WHERE ClientID = '" + Account + "';";
        auto ret = this->db.ReadMySQL(sql);
        return (ret.size() == 1 && ret.at(0).at(0) == Pwd);
    };

    if (!CheckIfAccountExist())
    {
        fprintf(stderr, "Account %s not exists.\n", Account.c_str());
        return false;
    }

    if (!CheckPassword())
    {
        fprintf(stderr, "Invalid password!\n");
        return false;
    }
    return true;
}

std::string Server::GetNickName(const std::string &ClientID)
{
    auto sql = "SELECT ClientNickname FROM ChatRoom WHERE ClientID = '" + ClientID + "';";
    auto nickname = this->db.ReadMySQL(sql).at(0).at(0);
    return nickname;
}

std::vector<std::string> Server::ShakeHandMsgParser(const std::string &msg_buf)
{
    size_t pos = msg_buf.find(']', 1);
    size_t idLen = atol(msg_buf.substr(1, pos - 1).c_str());
    std::string ID = msg_buf.substr(pos + 1, idLen);
    std::string Pwd = msg_buf.substr(pos + 1 + idLen, msg_buf.size());
    return {ID, Pwd};
}