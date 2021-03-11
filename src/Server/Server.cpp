#include "Server.h"

Server::Server() : myPool(maxThreadsNum)
{
    if (!this->Conn2DB(DATABASE_DOMAIN, "3306", DATABASE_ADMIN, DATABASE_NAME))
    {
        fprintf(stderr, "Failed to initialized database.\n");
        exit(EXIT_FAILURE);
    }

    if ((this->epfd = epoll_create(EPOLL_SIZE)) < 0)
    {
        fprintf(stderr, "epoll_create error\n");
        exit(SERVER_EPOLL_CREATE_ERROR);
    }

    if (!this->initOneServerListener(this->newUserListener, SERVER_NEW_USER_PORT))
    {
        fprintf(stderr, "Failed to initialize Server.\n");
        exit(EXIT_FAILURE);
    }
}

bool Server::initOneServerListener(int &listener, const char *port)
{
    addrinfo *ret, *cur, hints;
    bzero(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;     /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Stream socket */
    hints.ai_flags = AI_PASSIVE;     /* For wildcard IP address */
    hints.ai_protocol = 0;           /* Any protocol */
    hints.ai_canonname = nullptr;
    hints.ai_addr = nullptr;
    hints.ai_next = nullptr;

    auto err = getaddrinfo(nullptr, port, &hints, &ret);
    if (err != 0)
    {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(err));
        freeaddrinfo(ret);
        return false;
    }
    const int reuseAddr = 1;
    linger mySetLinger = {.l_onoff = 1, .l_linger = 5};
    for (cur = ret; cur; cur = cur->ai_next)
    {
        if ((listener = socket(cur->ai_family, cur->ai_socktype, cur->ai_protocol)) < 0)
            continue;
        if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &reuseAddr, sizeof(reuseAddr)) < 0)
        {
            fprintf(stderr, "setsockopt error:%s\n", strerror(errno));
            close(listener);
            freeaddrinfo(ret);
            return false;
        }
        if (setsockopt(listener, SOL_SOCKET, SO_LINGER, &mySetLinger, sizeof(linger)) < 0)
        {
            fprintf(stderr, "setsockopt error:%s\n", strerror(errno));
            close(listener);
            freeaddrinfo(ret);
            return false;
        }

        /*这里不应该调用SO_SNDBUF和SO_RCVBUF并设置缓冲区大小为nZero = 0，设置此标志虽然取消了内核缓冲区到应用缓冲区的双向拷贝
        提高了性能，但是直接使用应用缓冲区相当于对其加了锁，即send()和recv()只能顺序执行，无法并发
        setsockopt(listener, SOL_SOCKET, SO_SNDBUF, &nZero, sizeof(nZero));
        setsockopt(listener, SOL_SOCKET, SO_RCVBUF, &nZero, sizeof(nZero));*/

        if (bind(listener, cur->ai_addr, cur->ai_addrlen) == 0)
            break;
        close(listener);
    }

    if (!cur)
    {
        fprintf(stderr, "bind error\n");
        close(listener);
        return false;
    }

    freeaddrinfo(ret);

    if (listen(listener, BACK_LOG) < 0)
    {
        fprintf(stderr, "listen error: %s\n", strerror(errno));
        close(listener);
        return false;
    }

    fprintf(stdout, "Start to listen on local port: %s\n", port);
    addfd(this->epfd, listener, true);
    return true;
}

bool Server::initAllServerListener(std::list<std::pair<int, const char *>> &Listener_Port_List)
{
    bool flag = true;
    for (auto &iter : Listener_Port_List)
        flag &= this->initOneServerListener(iter.first, iter.second);
    return flag;
}

bool Server::Conn2DB(const char *db_domain, const char *port, const char *db_account, const char *db_name)
{
    char IP[128]{0};
    if (!Domain2IP(db_domain, SOCK_STREAM, IP, sizeof(IP)))
        fprintf(stderr, "Can't to resolve database's IP.\n");

    if (!this->db.ConnectMySQL(IP, db_account, db_name, atoi(port)))
    {
        fprintf(stderr, "Can't Connect to database at %s.\n", IP);
        return false;
    }
    return true;
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
            if (events[i].data.fd == this->newUserListener)
            {
                auto clientfd = accept(this->newUserListener, nullptr, nullptr);
                if (clientfd < 0)
                {
                    fprintf(stderr, "Accept error\n");
                    continue;
                }
                fprintf(stdout, "Accept New Connection.\n");
                addfd(this->epfd, clientfd, true);
            }
            else if (events[i].events & EPOLLIN)
            {
                auto clientfd = events[i].data.fd;

                char generalizedMessageToRecv[maxBufToMalloc]{0};

            READ_AGAIN:
                bzero(generalizedMessageToRecv, maxBufToMalloc);
                auto recvRet = recv(clientfd, reinterpret_cast<void *>(generalizedMessageToRecv), maxBufToMalloc, 0);
                if (!recvRet)
                {
                    struct epoll_event ev;
                    ev.events = EPOLLIN;
                    ev.data.fd = clientfd;
                    epoll_ctl(this->epfd, EPOLL_CTL_DEL, clientfd, &ev);
                    shutdown(clientfd, SHUT_RDWR);

                    auto ClosedAccount = this->Fd2_ID_Nickname[clientfd].first.c_str();
                    fprintf(stdout, "Clientfd %d (Account is %s) crashed. Now %lu client(s) online.\n", clientfd, ClosedAccount, this->client_list.size() - 1);
                    this->RemoveMappingInfo(clientfd);
                    continue;
                }
                else if (recvRet < 0)
                    if (errno == EAGAIN)
                        goto READ_AGAIN;

                switch (reinterpret_cast<MessageType *>(generalizedMessageToRecv)->OperCode)
                {
                case CLIENT_LOGIN_MSG:
                {
                    auto processerWhenNewLogin = [this, &clientfd, &generalizedMessageToRecv]() -> ssize_t {
                        sockaddr_in client_address;
                        socklen_t client_addrLength = sizeof(sockaddr_in);
                        getpeername(clientfd, reinterpret_cast<sockaddr *>(&client_address), &client_addrLength);
                        char Client_IP[128]{0};
                        inet_ntop(client_address.sin_family, &client_address.sin_addr, Client_IP, sizeof(Client_IP));
                        auto thisIdentity = reinterpret_cast<ClientLoginMessageType *>(generalizedMessageToRecv);
                        auto authVerifyStatusCode = this->AccountVerification(thisIdentity->ID, thisIdentity->Password);

                        if (authVerifyStatusCode != CLIENT_CHECK_SUCCESS)
                        {
                            switch (authVerifyStatusCode)
                            {
                            case CLIENT_ID_NOT_EXIST:
                                fprintf(stderr, "Account Verification Failed! Account %s not exist!\n", thisIdentity->ID);
                                break;
                            case WRONG_CLIENT_PASSWORD:
                                fprintf(stderr, "Account Verification Failed! Account %s exists but wrong password!\n", thisIdentity->ID);
                                break;
                            case DUPLICATED_LOGIN:
                                fprintf(stderr, "Denied a duplicated connection of %s\n", thisIdentity->ID);
                                break;
                            default:
                                break;
                            }
                        }
                        //发送给客户端登录状态码
                        ssize_t retCode{0};
                        if ((retCode = this->SendLoginStatus(clientfd, authVerifyStatusCode)) < 0)
                        {
                            close(clientfd);
                            fprintf(stderr, "Send login code error! Ignore connection from %s:%d\n", Client_IP, ntohs(client_address.sin_port));
                            return retCode;
                        }
                        if (authVerifyStatusCode != CLIENT_CHECK_SUCCESS)
                        {
                            close(clientfd);
                            return retCode;
                        }
                        //至此身份核查已通过,查数据库完善信息，并添加信息到系统文件描述表和映射表
                        this->AddMappingInfo(clientfd, thisIdentity->ID);
                        addfd(this->epfd, clientfd, true);
                        fprintf(stdout, "Connection from %s:%d, Account is %s, clientfd = %d, Now %lu client(s) online\n", Client_IP, ntohs(client_address.sin_port), thisIdentity->ID, clientfd, this->client_list.size());

                        if ((retCode = this->SendWelcomeMsg(clientfd)) < 0)
                        {
                            fprintf(stderr, "Send welcome message to %s:%d error\n", Client_IP, ntohs(client_address.sin_port));
                            this->MakeSomeoneOffline(clientfd, false);
                            return retCode;
                        }
                        return retCode;
                    };
                    this->ProcessingFunctionRet.emplace_back(this->myPool.enqueue(processerWhenNewLogin));
                    break;
                }

                case GROUP_MSG:
                {
                    this->ProcessingFunctionRet.emplace_back(this->myPool.enqueue([this, &clientfd, &generalizedMessageToRecv] { return this->SendBroadcastMsg(clientfd, reinterpret_cast<const ChatMessageType *>(generalizedMessageToRecv)->Msg); }));
                    break;
                }

                case PRIVATE_MSG:
                {
                    this->ProcessingFunctionRet.emplace_back(this->myPool.enqueue([this, &generalizedMessageToRecv, &clientfd]() -> ssize_t {
                        auto PrivateMsgPtr = reinterpret_cast<const ChatMessageType *>(generalizedMessageToRecv);
                        return (this->ID_fd.find(PrivateMsgPtr->Whom) == this->ID_fd.end()) ? -1 : this->SendPrivateMsg(clientfd, this->ID_fd[PrivateMsgPtr->Whom], PrivateMsgPtr->Msg);
                    }));

                    break;
                }

                case REQUEST_ONLINE_LIST:
                {
                    this->ProcessingFunctionRet.emplace_back(this->myPool.enqueue([this, &clientfd]() -> ssize_t { return this->SendOnlineList(clientfd); }));
                    break;
                }

                case REQUEST_NORMAL_OFFLINE:
                {
                    this->ProcessingFunctionRet.emplace_back(this->myPool.enqueue([this, &clientfd]() -> ssize_t {
                        auto sendStatus = this->SendAcceptNormalOffline(clientfd);
                        fprintf(stdout, "Clientfd %d (Account is %s) quited normally. Now %lu client(s) online.\n", clientfd, this->Fd2_ID_Nickname[clientfd].first.c_str(), this->client_list.size() - 1);
                        this->MakeSomeoneOffline(clientfd, false);
                        return sendStatus;
                    }));
                }

                default:
                    break;
                }
            }
            else
                fprintf(stdout, "Something wrong happened.\n");
        }
    }
    Close();
}

void Server::Close() const
{
    close(this->newUserListener);
    close(epfd);
}

void Server::AddMappingInfo(const int clientfd, const std::string &ID)
{
    //根据账号ID查数据库得到账号昵称
    auto nickname = this->GetNickName(ID);
    //写入文件描述符与账号信息的映射表
    this->Fd2_ID_Nickname.insert({clientfd, {ID, nickname}});

    //写入在线账号ID表
    this->ID_fd.insert({ID, clientfd});
    //服务端用list保存用户连接
    this->client_list.push_back(clientfd);
}

void Server::RemoveMappingInfo(const int clientfd)
{
    auto closedAccount = this->Fd2_ID_Nickname.find(clientfd)->second.first;
    //删除文件描述符->账号信息映射表
    this->Fd2_ID_Nickname.erase(this->Fd2_ID_Nickname.find(clientfd));

    //删除在线账号列表表项
    this->ID_fd.erase(this->ID_fd.find(closedAccount));
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
    ServerLoginCodeMessageType loginMessage;

    bzero(&loginMessage, sizeof(ServerLoginCodeMessageType));
    loginMessage.OperCode = SERVER_LOGIN_MSG;
    loginMessage.CheckCode = authVerifyStatusCode;

    return send(clientfd, reinterpret_cast<const void *>(&loginMessage), sizeof(ServerLoginCodeMessageType), 0);
}

ssize_t Server::SendWelcomeMsg(int clientfd)
{
    ChatMessageType welcomeMessageToSend;
    bzero(&welcomeMessageToSend, sizeof(ChatMessageType));
    auto SYSTEM_MSG = "System Message";

    welcomeMessageToSend.OperCode = PRIVATE_MSG;
    snprintf(welcomeMessageToSend.Msg, BUF_SIZE, SERVER_WELCOME, this->Fd2_ID_Nickname[clientfd].second.c_str());
    snprintf(welcomeMessageToSend.Whom, MAX_ACCOUNT_LEN, SYSTEM_MSG);

    return send(clientfd, reinterpret_cast<void *>(&welcomeMessageToSend), sizeof(ChatMessageType), 0);
}

ssize_t Server::SendBroadcastMsg(const int clientfd, const char *MessageToSend)
{
    ChatMessageType broadcastMessageToSend;
    bzero(&broadcastMessageToSend, sizeof(ChatMessageType));

    broadcastMessageToSend.OperCode = GROUP_MSG;
    snprintf(broadcastMessageToSend.Msg, BUF_SIZE, MessageToSend);
    snprintf(broadcastMessageToSend.Whom, MAX_ACCOUNT_LEN, this->Fd2_ID_Nickname[clientfd].second.c_str());

    for (const auto &iter : this->client_list)
        if (iter != clientfd)
            if (send(iter, reinterpret_cast<const void *>(&broadcastMessageToSend), sizeof(ChatMessageType), 0) < 0)
                return -1;

    return 0;
}

ssize_t Server::SendPrivateMsg(const int fd_from, const int fd_to, const char *MessageToSend)
{
    ChatMessageType privateMessageToSend;
    bzero(&privateMessageToSend, sizeof(ChatMessageType));

    privateMessageToSend.OperCode = PRIVATE_MSG;
    snprintf(privateMessageToSend.Msg, BUF_SIZE, MessageToSend);
    snprintf(privateMessageToSend.Whom, MAX_ACCOUNT_LEN, this->Fd2_ID_Nickname[fd_from].second.c_str());

    return send(fd_to, reinterpret_cast<const void *>(&privateMessageToSend), sizeof(ChatMessageType), 0);
}

ssize_t Server::SendOnlineList(const int clientfd)
{
    OnlineListMessageType OnlineListMessageToSend;
    bzero(&OnlineListMessageToSend, sizeof(OnlineListMessageType));
    std::string AccountsSplitByComma;
    for (const auto &iter : this->ID_fd)
    {
        AccountsSplitByComma += iter.first;
        AccountsSplitByComma.append(",");
    }
    auto accountPtr = AccountsSplitByComma.c_str();

    OnlineListMessageToSend.OperCode = REPLY_ONLINE_LIST;
    OnlineListMessageToSend.OnLineNum = this->client_list.size();
    memcpy(OnlineListMessageToSend.List, accountPtr, std::min(BUF_SIZE, strlen(accountPtr) - 1));

    return send(clientfd, reinterpret_cast<const void *>(&OnlineListMessageToSend), sizeof(OnlineListMessageType), 0);
}

ssize_t Server::SendAcceptNormalOffline(const int clientfd)
{
    MessageType acceptNormalOfflineMessageToSend;
    bzero(&acceptNormalOfflineMessageToSend, sizeof(MessageType));
    acceptNormalOfflineMessageToSend.OperCode = REPLY_NORMAL_OFFLINE;
    return send(clientfd, reinterpret_cast<const void *>(&acceptNormalOfflineMessageToSend), sizeof(MessageType), 0);
}

bool Server::CheckIsDuplicatedLoggin(const std::string &ID)
{
    auto iter = this->ID_fd.find(ID);
    return !(iter == this->ID_fd.end());
}

bool Server::CheckIfAccountExistInMySQL(const std::string &ID)
{
    const std::string sql = "SELECT ClientID FROM ChatRoom WHERE ClientID = '" + ID + "';";
    return (this->db.ReadMySQL(sql).size() == 1);
}

bool Server::CheckIsAccountPasswordMatch(const std::string &ID, const std::string &Password)
{
    const std::string sql = "SELECT ClientPassword FROM ChatRoom WHERE ClientID = '" + ID + "';";
    auto ret = this->db.ReadMySQL(sql);
    return (ret.size() == 1 && ret.at(0).at(0) == Password);
}

LoginStatusCodeType Server::AccountVerification(const char *ID, const char *Password)
{
    if (!this->CheckIfAccountExistInMySQL(ID))
        return CLIENT_ID_NOT_EXIST;

    if (!this->CheckIsAccountPasswordMatch(ID, Password))
        return WRONG_CLIENT_PASSWORD;

    if (this->CheckIsDuplicatedLoggin(ID))
        return DUPLICATED_LOGIN;

    return CLIENT_CHECK_SUCCESS;
}