#include "Client.h"
#include "Common.h"
#include <ctime>
#include <iostream>
#include <sys/prctl.h>
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
    memcpy(this->myID, id, strlen(id));
    memcpy(this->myPassword, pwd, strlen(pwd));

    addrinfo hints{}, *ret, *cur;
    bzero(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC; /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_ALL;
    hints.ai_protocol = 0;
    auto err = getaddrinfo(SERVER_DOMAIN, SERVER_NEW_USER_PORT, &hints, &ret);
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
    this->TellMyIdentity();
    this->RecvLoginStatus();
}

//向服务器发送输入的ID和密码
void Client::TellMyIdentity()
{
    bzero(this->sendBuf, sizeof(ClientLoginMessageType));
    reinterpret_cast<ClientLoginMessageType *>(this->sendBuf)->OperCode = CLIENT_LOGIN_MSG;
    memcpy(reinterpret_cast<ClientLoginMessageType *>(this->sendBuf)->ID, this->myID, strlen(this->myID));
    memcpy(reinterpret_cast<ClientLoginMessageType *>(this->sendBuf)->Password, this->myPassword, strlen(this->myPassword));

    if (send(this->sock, reinterpret_cast<const void *>(this->sendBuf), sizeof(ClientLoginMessageType), 0) < 0)
    {
        fprintf(stderr, "Send authentication error\n");
        exit(CLIENT_SEND_ERROR);
    }
}

void Client::RecvLoginStatus()
{
    auto old_fd_status = fcntl(this->sock, F_GETFD, 0);
    fcntl(this->sock, F_SETFL, fcntl(this->sock, F_GETFD, 0) & ~O_NONBLOCK);
    auto len = recv(this->sock, reinterpret_cast<ServerLoginCodeMessageType *>(this->recvBuf), sizeof(ServerLoginCodeMessageType), 0);
    fcntl(this->sock, F_SETFL, old_fd_status);

    if (len < 0 || reinterpret_cast<ServerLoginCodeMessageType *>(this->recvBuf)->OperCode != SERVER_LOGIN_MSG)
    {
        fprintf(stderr, "Error occurs, %s\n", strerror(errno));
        exit(CLIENT_RECV_ERROR);
    }

    switch (reinterpret_cast<ServerLoginCodeMessageType *>(this->recvBuf)->CheckCode)
    {
    case CLIENT_ID_NOT_EXIST:
        fprintf(stderr, "Account not exists, please check your account\n");
        exit(CLIENT_ID_NOT_EXIST);
    case WRONG_CLIENT_PASSWORD:
        fprintf(stderr, "Account's password error, please check your password\n");
        exit(WRONG_CLIENT_PASSWORD);
    case DUPLICATED_LOGIN:
        fprintf(stderr, "Account has been online!\n");
        exit(DUPLICATED_LOGIN);
    default:
        break;
    }
}

void Client::Start()
{
    this->Connect();
    fprintf(stdout, " ______________________________________________________________\n");
    fprintf(stdout, "|_______________________Message Type Code______________________|\n");
    fprintf(stdout, "|                                                              |\n");
    fprintf(stdout, "| Group Message -------------------------------------------> 1 |\n");
    fprintf(stdout, "| Private Message -----------------------------------------> 2 |\n");
    fprintf(stdout, "| Get Online List -----------------------------------------> 3 |\n");
    fprintf(stdout, "| Logout the chatroom -------------------------------------> 4 |\n");
    fprintf(stdout, "|______________________________________________________________|\n");
    static struct epoll_event events[2];
    signal(SIGCHLD, handlerSIGCHLD);
    fflush(nullptr);

    if ((this->pid = fork()) < 0)
    {
        fprintf(stderr, "fork() error\n");
        exit(CLIENT_FORK_ERROR);
    }
    else if (!this->pid)
    {
        prctl(PR_SET_PDEATHSIG, SIGKILL);
        close(this->pipe_fd[0]);
        while (this->isClientWork)
        {
            //TODO:这里的错误输入处理机制有问题
            int selectCode;
            fflush(stdin);
            std::cin >> selectCode;
            if (selectCode > 4 || !selectCode)
            {
                std::cout << "Invalid selection: " << selectCode << std::endl;
                continue;
            }

            bzero(this->sendBuf, maxBufToMalloc);
            size_t writeBytes{0};
            switch (selectCode)
            {
            case 4: //Logout
                reinterpret_cast<LogoutMessageType *>(this->sendBuf)->OperCode = REQUEST_NORMAL_OFFLINE;
                writeBytes = sizeof(LogoutMessageType);
                break;
            case 3: //Get Online List
                reinterpret_cast<OnlineListRequestType *>(this->sendBuf)->OperCode = REQUEST_ONLINE_LIST;
                writeBytes = sizeof(OnlineListRequestType);
                break;
            case 2: //Private Message
                reinterpret_cast<ChatMessageType *>(this->sendBuf)->OperCode = PRIVATE_MSG;
                fprintf(stdout, "Send to account: ");
                std::cin >> reinterpret_cast<ChatMessageType *>(this->sendBuf)->Whom;
                fprintf(stdout, "给%s的私聊消息: ", reinterpret_cast<ChatMessageType *>(this->sendBuf)->Whom);
                getchar();
                std::cin.getline(reinterpret_cast<ChatMessageType *>(this->sendBuf)->Msg, BUF_SIZE);
                writeBytes = sizeof(ChatMessageType);
                break;
            case 1: //Group Message
                reinterpret_cast<ChatMessageType *>(this->sendBuf)->OperCode = GROUP_MSG;
                getchar();
                fprintf(stdout, "要发送的群聊消息: ");
                std::cin.getline(reinterpret_cast<ChatMessageType *>(this->sendBuf)->Msg, BUF_SIZE);
                writeBytes = sizeof(ChatMessageType);
                break;
            default:
                break;
            }
            if (write(this->pipe_fd[1], reinterpret_cast<const void *>(this->sendBuf), writeBytes) < 0)
                exit(CLIENT_WRITE_ERROR);
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
                bzero(this->recvBuf, maxBufToMalloc);
                if (events[i].data.fd == this->sock) //服务端发来消息
                {
                    if (!recv(this->sock, reinterpret_cast<void *>(this->recvBuf), maxBufToMalloc, 0))
                    {
                        this->isClientWork = false;
                        fprintf(stderr, "Server closed.\n");
                    }
                    else
                    {
                        auto curTime = getTime();
                        switch (reinterpret_cast<MessageType *>(recvBuf)->OperCode)
                        {
                        case GROUP_MSG:
                            fprintf(stdout, "[%s] %s >>> %s\n", curTime.c_str(), reinterpret_cast<ChatMessageType *>(this->recvBuf)->Whom, reinterpret_cast<ChatMessageType *>(this->recvBuf)->Msg);
                            break;
                        case PRIVATE_MSG:
                            fprintf(stdout, "[%s] %s(private) >>> %s\n", curTime.c_str(), reinterpret_cast<ChatMessageType *>(this->recvBuf)->Whom, reinterpret_cast<ChatMessageType *>(this->recvBuf)->Msg);
                            break;
                        case REPLY_ONLINE_LIST:
                            fprintf(stdout, "[%s] Accounts Online num: %lu\n", curTime.c_str(), reinterpret_cast<OnlineListMessageType *>(this->recvBuf)->OnLineNum);
                            fprintf(stdout, "[%s] Accounts Online List: %s\n", curTime.c_str(), reinterpret_cast<OnlineListMessageType *>(this->recvBuf)->List);
                            break;
                        case REPLY_NORMAL_OFFLINE:
                            fprintf(stdout, "[%s] Prepared to be offline.\n", curTime.c_str());
                            exit(EXIT_SUCCESS);
                            break;
                        default:
                            break;
                        }
                    }
                }
                else //子进程写入事件发生，父进程处理并发送服务端
                {
                    if (read(events[i].data.fd, reinterpret_cast<void *>(this->sendBuf), maxBufToMalloc) == 0) //子进程死了
                        this->isClientWork = false;
                    else
                        send(this->sock, reinterpret_cast<void *>(this->sendBuf), maxBufToMalloc, 0);
                }
            }
        }
    }
}
Client::~Client() { close(this->pipe_fd[!this->pid ? 1 : 0]); }
