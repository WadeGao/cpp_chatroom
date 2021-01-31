#include "Server.h"
#include <iostream>

Server::Server() : listener(0), epfd(0)
{
    //服务器初始化
    serverAddr.sin_family = PF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("0.0.0.0");
    serverAddr.sin_port = htons(SERVER_PORT);

    //取得分布式数据库服务器IP地址
    auto host = gethostbyname(DATABASE_DOMAIN);
    if (!host)
    {
        fprintf(stderr, "Can't resolve Database domain: %s\n", DATABASE_DOMAIN);
        exit(EXIT_FAILURE);
    }
    for (size_t i = 0; host->h_addr_list[i]; i++)
        this->DB_IP_List.push_back(inet_ntoa(*(struct in_addr *)host->h_addr_list[i]));
}

Server::~Server() = default;

void Server::Init()
{
    fprintf(stdout, "Init Server...\n");

    if ((listener = socket(serverAddr.sin_family, SOCK_STREAM, 0)) < 0)
    {
        fprintf(stderr, "listener error\n");
        exit(EXIT_FAILURE);
    }
    //fdAutoCloser(listener);

    if (bind(listener, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        fprintf(stderr, "bind error: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (listen(listener, 5) < 0)
    {
        fprintf(stderr, "listen error");
        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "Start to listen local port: %u\n", SERVER_PORT);

    if ((epfd = epoll_create(EPOLL_SIZE)) < 0)
    {
        fprintf(stderr, "epoll_create() error\n");
        exit(EXIT_FAILURE);
    }
    //fdAutoCloser(epfd);
    addfd(epfd, listener, true);

    //连接到数据库
    //TODO:MySQL服务器负载均衡
    if (!this->db.ConnectMySQL(this->DB_IP_List.at(LoadBalancer(this->DB_IP_List.size())), DATABASE_ADMIN, DATABASE_NAME, true, DATABASE_PWD))
    {
        fprintf(stderr, "Can't Connect to Database.\n");
        exit(FAIL_CONNECT_DB);
    }
}

void Server::Start()
{
    //TODO: 搞文件传输
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
            char msg[BUF_SIZE] = {0};
            auto sockfd = events[i].data.fd;
            if (sockfd == listener)
            {
                struct sockaddr_in client_address;
                socklen_t client_addrLength = sizeof(struct sockaddr_in);
                auto clientfd = accept(listener, (struct sockaddr *)&client_address, &client_addrLength);

                //接收首次通信时由客户端告知的连接用户身份信息
                char ID_Pwd_buf[BUF_SIZE] = {0};
                //TODO:这里，recv被阻塞，等待新连接用户的消息
                auto len = recv(clientfd, ID_Pwd_buf, BUF_SIZE, 0);
                if (!len)
                    continue;

                auto vectorID_Pwd = this->ShakeHandMsgParser(ID_Pwd_buf);
                auto authVerifyStatus = this->AccountVerification(vectorID_Pwd.at(0), vectorID_Pwd.at(1));

                //发送给客户端错误状态码
                bzero(msg, BUF_SIZE);
                sprintf(msg, DENY_SERVE, char(authVerifyStatus));
                if (send(clientfd, msg, strlen(msg), 0) < 0)
                {
                    fprintf(stderr, "send() auth info error\n");
                    Close();
                    exit(EXIT_FAILURE);
                }
                //打印错误记录
                if (authVerifyStatus != CHECK_SUCCESS)
                {
                    close(clientfd);

                    switch (authVerifyStatus)
                    {
                    case CLIENTID_NOT_EXIST:
                        fprintf(stderr, "\033[31mAccount Verification Failed! Account %s not exist!\n\033[0m", vectorID_Pwd.at(0).c_str());
                        break;
                    case WRONG_CLIENT_PASSWORD:
                        fprintf(stderr, "\033[31mAccount Verification Failed! Account %s exists but wrong password!\n\033[0m", vectorID_Pwd.at(0).c_str());
                        break;
                    case DUPLICATED_LOGIN:
                        fprintf(stderr, "\033[31mDenied a duplicated connection of %s\n\033[0m", vectorID_Pwd.at(0).c_str());
                        break;
                    default:
                        break;
                    }

                    continue;
                }

                //至此身份核查已通过
                //查数据库完善信息，并添加信息到系统文件描述表和映射表
                this->AddMappingInfo(clientfd, vectorID_Pwd.at(0));

                addfd(epfd, clientfd, true);
                fprintf(stdout, "\033[31mClient connection from %s:%u, clientfd = %d\n\033[0m", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port), clientfd);
                fprintf(stdout, "Add new clientfd = %d to epoll. Now there are %lu client(s) int the chat room\n", clientfd, client_list.size());
                fprintf(stdout, "Welcome Message\n");

                bzero(msg, BUF_SIZE);
                sprintf(msg, SERVER_WELCOME, this->Fd2_ID_Nickname.find(clientfd)->second.second.c_str());
                if (send(clientfd, msg, strlen(msg), 0) < 0)
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

ssize_t Server::SendBroadcastMsg(const int clientfd)
{
    char buf[BUF_SIZE] = {0}, msg[BUF_SIZE] = {0};
    fprintf(stdout, "read from client %d\n", clientfd);

    auto len = recv(clientfd, buf, BUF_SIZE, 0);

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
        bzero(msg, BUF_SIZE);
        sprintf(msg, SERVER_MSG, this->Fd2_ID_Nickname.find(clientfd)->second.second.c_str(), buf);
        for (const auto &iter : client_list)
        {
            if (iter != clientfd)
                if (send(iter, msg, strlen(msg), 0) < 0)
                    return -1;
            //this->client_list.remove(clientfd);
        }
    }
    return len;
}

void Server::Close() const
{
    close(listener);
    close(epfd);
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

bool Server::IsDuplicatedLoggin(const std::string &ID)
{
    auto iter = this->If_Duplicated_Loggin.find(ID);
    return !(iter == this->If_Duplicated_Loggin.end()) && iter->second;
}

size_t Server::AccountVerification(const std::string &Account, const std::string &Pwd)
{
    //账号不存在
    //账号密码错误
    //重复登录
    auto CheckIfAccountExist = [&Account, this]() -> bool {
        const std::string sql = "SELECT ClientID FROM ChatRoom WHERE ClientID = '" + Account + "';";
        return (this->db.ReadMySQL(sql).size() == 1);
    };
    auto CheckPassword = [&Account, &Pwd, this]() -> bool {
        const std::string sql = "SELECT ClientPassword FROM ChatRoom WHERE ClientID = '" + Account + "';";
        auto ret = this->db.ReadMySQL(sql);
        return (ret.size() == 1 && ret.at(0).at(0) == Pwd);
    };

    //账号不存在
    if (!CheckIfAccountExist())
        return CLIENTID_NOT_EXIST;

    //密码错误
    if (!CheckPassword())
        return WRONG_CLIENT_PASSWORD;

    //账号重复登录
    if (this->IsDuplicatedLoggin(Account))
        return DUPLICATED_LOGIN;

    return CHECK_SUCCESS;
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