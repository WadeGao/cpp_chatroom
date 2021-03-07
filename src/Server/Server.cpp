#include "Server.h"
#include <iostream>

Server::Server()
{
    this->Conn2DB(DATABASE_DOMAIN, "3306", DATABASE_ADMIN, DATABASE_NAME);

    addrinfo *ret, *cur, hints{};
    bzero(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;     /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Stream socket */
    hints.ai_flags = AI_PASSIVE;     /* For wildcard IP address */
    hints.ai_protocol = 0;           /* Any protocol */
    hints.ai_canonname = nullptr;
    hints.ai_addr = nullptr;
    hints.ai_next = nullptr;
    auto err = getaddrinfo(nullptr, SERVER_PORT, &hints, &ret);
    if (err != 0)
    {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(err));
        exit(SERVER_GETADDRINFO_ERROR);
    }

    const int reuseAddr = 1;
    for (cur = ret; cur; cur = cur->ai_next)
    {
        if ((this->listener = socket(cur->ai_family, cur->ai_socktype, cur->ai_protocol)) < 0)
            continue;

        if (setsockopt(this->listener, SOL_SOCKET, SO_REUSEADDR, &reuseAddr, sizeof(reuseAddr)) < 0)
        {
            fprintf(stderr, "setsockopt reuse addr error\n");
            exit(SERVER_SETSOCKOPT_ERROR);
        }

        if (bind(this->listener, cur->ai_addr, cur->ai_addrlen) == 0)
            break;

        close(this->listener);
    }

    if (!cur)
    {
        fprintf(stderr, "bind error\n");
        exit(BIND_ERROR);
    }
    freeaddrinfo(ret);

    if (listen(this->listener, BACK_LOG) < 0)
    {
        fprintf(stderr, "listen error: %s\n", strerror(errno));
        exit(LISTEN_ERROR);
    }

    fprintf(stdout, "Start to listen on local port: %s\n", SERVER_PORT);

    if ((this->epfd = epoll_create(EPOLL_SIZE)) < 0)
        exit(SERVER_EPOLL_CREATE_ERROR);

    addfd(this->epfd, this->listener, true);
}

void Server::Conn2DB(const char *db_domain, const char *port, const char *db_account, const char *db_name)
{
    addrinfo *ret, *cur, hints{};
    bzero(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC; /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_ALL;
    hints.ai_protocol = 0; /* Any protocol */

    auto err = getaddrinfo(db_domain, port, &hints, &ret);
    if (err != 0)
    {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(err));
        exit(SERVER_GETADDRINFO_ERROR);
    }

    char IP[128]{0};
    for (cur = ret; cur; cur = cur->ai_next)
    {
        bzero(&IP, sizeof(IP));
        if (cur->ai_family == AF_INET6)
        {
            auto sockAddr_IPv6 = reinterpret_cast<sockaddr_in6 *>(cur->ai_addr);
            if (inet_ntop(cur->ai_family, &sockAddr_IPv6->sin6_addr, IP, sizeof(IP)))
                break;
            else
                continue;
        }
        else if (cur->ai_family == AF_INET)
        {
            auto sockAddr_IPv4 = reinterpret_cast<sockaddr_in *>(cur->ai_addr);
            if (inet_ntop(cur->ai_family, &sockAddr_IPv4->sin_addr, IP, sizeof(IP)))
                break;
            else
                continue;
        }
        else
        {
            fprintf(stderr, "protocol unsupported\n");
            exit(UNSUPPORTED_PROTOCOL);
        }
    }
    if (!cur)
    {
        fprintf(stderr, "inet_ntop error\n");
        exit(SERVER_INET_NTOP_ERROR);
    }
    freeaddrinfo(ret);

    if (!this->db.ConnectMySQL(IP, db_account, db_name, atoi(port)))
    {
        fprintf(stderr, "Can't Connect to Database.\n");
        exit(SERVER_FAIL_CONNECT_DB);
    }
}

void Server::Start()
{
    struct epoll_event events[EPOLL_SIZE];
    while (true)
    {
        auto epoll_events_count = epoll_wait(epfd, events, EPOLL_SIZE, -1);
        if (epoll_events_count < 0)
        {
            fprintf(stderr, "epoll fail\n");
            break;
        }

        for (decltype(epoll_events_count) i = 0; i < epoll_events_count; i++)
        {
            auto sockfd = events[i].data.fd;
            if (sockfd == this->listener)
            {
                struct sockaddr_in client_address
                {
                };
                socklen_t client_addrLength = sizeof(struct sockaddr_in);
                auto clientfd = accept(this->listener, reinterpret_cast<sockaddr *>(&client_address), &client_addrLength);
                if (clientfd < 0)
                {
                    if (errno == EINTR)
                        continue;
                    else
                    {
                        fprintf(stderr, "\033[31maccept error\n\033[0m");
                        continue;
                    }
                }

                ConnInfo myConnInfo;
                if (getnameinfo(reinterpret_cast<sockaddr *>(&client_address), client_addrLength, myConnInfo.Host, MAX_CLIENT_IP_LEN, myConnInfo.Serv, MAX_CLIENT_PORT_LEN, NI_NUMERICHOST | NI_NUMERICSERV) != 0)
                {
                    fprintf(stderr, "getnameinfo error.\n");
                    exit(SERVER_GETNAMEINFO_ERROR);
                }

                //接收首次通信时由客户端告知的连接用户身份信息
                ClientIdentityType thisConnIdentity{};
                if (!recv(clientfd, reinterpret_cast<void *>(&thisConnIdentity), sizeof(ClientIdentityType), 0))
                    continue;
                auto authVerifyStatusCode = this->AccountVerification(thisConnIdentity);

                //打印错误记录
                if (authVerifyStatusCode != CLIENT_CHECK_SUCCESS)
                {
                    //close(clientfd);

                    switch (authVerifyStatusCode)
                    {
                    case CLIENT_ID_NOT_EXIST:
                        fprintf(stderr, "\033[31mAccount Verification Failed! Account %s not exist!\n\033[0m", thisConnIdentity.ID);
                        break;
                    case WRONG_CLIENT_PASSWORD:
                        fprintf(stderr, "\033[31mAccount Verification Failed! Account %s exists but wrong password!\n\033[0m", thisConnIdentity.ID);
                        break;
                    case DUPLICATED_LOGIN:
                        fprintf(stderr, "\033[31mDenied a duplicated connection of %s\n\033[0m", thisConnIdentity.ID);
                        break;
                    default:
                        break;
                    }
                    //continue;
                }

                //发送给客户端登录状态码
                if (this->SendLoginStatus(clientfd, authVerifyStatusCode) < 0)
                {
                    close(clientfd);
                    fprintf(stderr, "\033[31mSend login code error! Ignore connection from %s:%s\n\033[0m", myConnInfo.Host, myConnInfo.Serv);
                    continue;
                }

                if (authVerifyStatusCode != CLIENT_CHECK_SUCCESS)
                {
                    close(clientfd);
                    continue;
                }

                //至此身份核查已通过,查数据库完善信息，并添加信息到系统文件描述表和映射表
                this->AddMappingInfo(clientfd, thisConnIdentity.ID);

                addfd(this->epfd, clientfd, true);

                fprintf(stdout, "\033[31mConnection from %s:%s, Account is %s, clientfd = %d, Now %lu client(s) online\n\033[0m", myConnInfo.Host, myConnInfo.Serv, thisConnIdentity.ID, clientfd, this->client_list.size());

                if (this->SendWelcomeMsg(clientfd) < 0)
                {
                    fprintf(stderr, "Send welcome message to %s:%s error\n", myConnInfo.Host, myConnInfo.Serv);
                    this->MakeSomeoneOffline(clientfd, false);
                }
            }
            else
            {
                MessageType generalizedMessageToRecv;
                bzero(&generalizedMessageToRecv, sizeof(MessageType));

                auto len = recv(sockfd, reinterpret_cast<void *>(&generalizedMessageToRecv), sizeof(MessageType), 0);
                if (!len)
                {
                    auto ClosedAccount = this->Fd2_ID_Nickname[sockfd].first.c_str();
                    fprintf(stdout, "\033[31mClientfd %d (Account is %s) crashed. Now %lu client(s) online.\n\033[0m", sockfd, ClosedAccount, this->client_list.size() - 1);
                    this->MakeSomeoneOffline(sockfd, false);
                    continue;
                }

                switch (generalizedMessageToRecv.OperCode)
                {
                case GROUP_MSG:
                    if (this->SendBroadcastMsg(sockfd, generalizedMessageToRecv.msg) < 0)
                        continue;
                    break;
                case PRIVATE_MSG:
                    //auto iter = this->ID_fd.find(generalizedMessageToRecv.msg_code.Whom);
                    if (this->ID_fd.find(generalizedMessageToRecv.msg_code.Whom) != this->ID_fd.end())
                        if (this->SendPrivateMsg(sockfd, this->ID_fd[generalizedMessageToRecv.msg_code.Whom], generalizedMessageToRecv.msg) < 0)
                            continue;
                    break;
                case REQUEST_ONLINE_LIST:
                    if (this->SendOnlineList(sockfd) < 0)
                        continue;
                    break;
                case REQUEST_NORMAL_OFFLINE:
                    if (this->SendAcceptNormalOffline(sockfd) < 0)
                        continue;
                    fprintf(stdout, "\033[31mClientfd %d (Account is %s) quited normally. Now %lu client(s) online.\n\033[0m", sockfd, this->Fd2_ID_Nickname[sockfd].first.c_str(), this->client_list.size() - 1);
                    this->MakeSomeoneOffline(sockfd, false);
                    break;
                default:
                    break;
                }
            }
        }
    }
    Close();
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
    this->Fd2_ID_Nickname.insert({clientfd, {ID_buf, nickname}});
    this->ID_fd.insert({ID_buf, clientfd});
    //写入在线账号ID表
    this->If_Duplicated_Loggin.insert({ID_buf, true});
    //服务端用list保存用户连接
    this->client_list.push_back(clientfd);
}

void Server::RemoveMappingInfo(const int clientfd)
{
    auto closedAccount = this->Fd2_ID_Nickname.find(clientfd)->second.first;
    //删除文件描述符->账号信息映射表
    this->Fd2_ID_Nickname.erase(this->Fd2_ID_Nickname.find(clientfd));
    this->ID_fd.erase(this->ID_fd.find(closedAccount));
    //删除在线账号列表表项
    this->If_Duplicated_Loggin.erase(this->If_Duplicated_Loggin.find(closedAccount));
    //将该账号对应的文件描述符从在线文件描述符集中删除
    this->client_list.remove(clientfd);
}

std::string Server::GetNickName(const std::string &ClientID)
{
    auto sql = "SELECT ClientNickname FROM ChatRoom WHERE ClientID = '" + ClientID + "';";
    auto nickname = this->db.ReadMySQL(sql).at(0).at(0);
    return nickname;
}

void Server::MakeSomeoneOffline(int clientfd, bool isForced)
{
    if (this->Fd2_ID_Nickname.find(clientfd) != this->Fd2_ID_Nickname.end())
    {
        if (isForced)
        { //TODO:怎么踢人
        }
        close(clientfd);
        this->RemoveMappingInfo(clientfd);
    }
}

ssize_t Server::SendLoginStatus(int clientfd, const LoginStatusCodeType authVerifyStatusCode)
{
    MessageType loginMessage;

    bzero(&loginMessage, sizeof(MessageType));
    loginMessage.OperCode = LOGIN_CODE_MSG;
    memcpy(&loginMessage.msg_code.Code, &authVerifyStatusCode, sizeof(authVerifyStatusCode));

    return send(clientfd, reinterpret_cast<const void *>(&loginMessage), sizeof(MessageType), 0);
}

ssize_t Server::SendWelcomeMsg(int clientfd)
{
    MessageType welcomeMessageToSend;
    bzero(&welcomeMessageToSend, sizeof(MessageType));
    welcomeMessageToSend.OperCode = WELCOME_WITH_IDENTITY_MSG;
    snprintf(welcomeMessageToSend.msg, BUF_SIZE, SERVER_WELCOME, this->Fd2_ID_Nickname[clientfd].second.c_str());
    return send(clientfd, reinterpret_cast<void *>(&welcomeMessageToSend), sizeof(MessageType), 0);
}

ssize_t Server::SendBroadcastMsg(const int clientfd, const char *MessageToSend)
{
    MessageType broadcastMessageToSend;
    bzero(&broadcastMessageToSend, sizeof(MessageType));

    broadcastMessageToSend.OperCode = GROUP_MSG;
    memcpy(broadcastMessageToSend.msg, MessageToSend, BUF_SIZE);
    snprintf(broadcastMessageToSend.msg_code.Whom, MAX_ACCOUNT_LEN, this->Fd2_ID_Nickname[clientfd].second.c_str());

    for (const auto &iter : this->client_list)
        if (iter != clientfd)
            if (send(iter, reinterpret_cast<const void *>(&broadcastMessageToSend), sizeof(MessageType), 0) < 0)
                return -1;

    return 0;
}

ssize_t Server::SendPrivateMsg(const int fd_from, const int fd_to, const char *MessageToSend)
{
    MessageType privateMessageToSend;
    bzero(&privateMessageToSend, sizeof(MessageType));
    privateMessageToSend.OperCode = PRIVATE_MSG;
    memcpy(privateMessageToSend.msg, MessageToSend, BUF_SIZE);
    snprintf(privateMessageToSend.msg_code.Whom, MAX_ACCOUNT_LEN, this->Fd2_ID_Nickname[fd_from].second.c_str());
    return send(fd_to, reinterpret_cast<const void *>(&privateMessageToSend), sizeof(MessageType), 0);
}

ssize_t Server::SendOnlineList(const int clientfd)
{
    //TODO:完成获取在线列表的实现
    MessageType OnlineListMessageToSend;
    bzero(&OnlineListMessageToSend, sizeof(MessageType));
    OnlineListMessageToSend.OperCode = REPLY_ONLINE_LIST;
    OnlineListMessageToSend.msg_code.online_num = this->client_list.size();

    std::string AccountsSplitByComma;
    for (const auto &iter : this->ID_fd)
    {
        AccountsSplitByComma += iter.first;
        AccountsSplitByComma += std::string(",");
    }

    auto accountPtr = AccountsSplitByComma.c_str();
    memcpy(OnlineListMessageToSend.msg, accountPtr, std::min(BUF_SIZE, strlen(accountPtr) - 1));
    return send(clientfd, reinterpret_cast<const void *>(&OnlineListMessageToSend), sizeof(MessageType), 0);
}

ssize_t Server::SendAcceptNormalOffline(const int clientfd)
{
    MessageType acceptNormalOfflineMessageToSend;
    bzero(&acceptNormalOfflineMessageToSend, sizeof(MessageType));
    acceptNormalOfflineMessageToSend.OperCode = ACCEPT_NORMAL_OFFLINE;
    return send(clientfd, reinterpret_cast<const void *>(&acceptNormalOfflineMessageToSend), sizeof(MessageType), 0);
}

bool Server::CheckIsDuplicatedLoggin(const std::string &ID)
{
    auto iter = this->If_Duplicated_Loggin.find(ID);
    return !(iter == this->If_Duplicated_Loggin.end()) && iter->second;
}

bool Server::CheckIfAccountExistInMySQL(const ClientIdentityType &thisConnIdentity)
{
    const std::string sql = "SELECT ClientID FROM ChatRoom WHERE ClientID = '" + std::string(thisConnIdentity.ID) + "';";
    return (this->db.ReadMySQL(sql).size() == 1);
}

bool Server::CheckIsAccountPasswordMatch(const ClientIdentityType &thisConnIdentity)
{
    const std::string sql = "SELECT ClientPassword FROM ChatRoom WHERE ClientID = '" + std::string(thisConnIdentity.ID) + "';";
    auto ret = this->db.ReadMySQL(sql);
    return (ret.size() == 1 && ret.at(0).at(0) == std::string(thisConnIdentity.Password));
}

LoginStatusCodeType Server::AccountVerification(const ClientIdentityType &thisConnIdentity)
{
    if (!this->CheckIfAccountExistInMySQL(thisConnIdentity))
        return CLIENT_ID_NOT_EXIST;

    if (!this->CheckIsAccountPasswordMatch(thisConnIdentity))
        return WRONG_CLIENT_PASSWORD;

    if (this->CheckIsDuplicatedLoggin(thisConnIdentity.ID))
        return DUPLICATED_LOGIN;

    return CLIENT_CHECK_SUCCESS;
}