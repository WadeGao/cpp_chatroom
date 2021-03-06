#include "Client.h"
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
    if (send(this->sock, reinterpret_cast<const void *>(&this->myIdentity), sizeof(ClientIdentityType), 0) < 0)
    {
        fprintf(stderr, "Send authentication error\n");
        //this->Close();
        exit(CLIENT_SEND_ERROR);
    }
}

void Client::RecvLoginStatus()
{
    auto old_fd_status = fcntl(this->sock, F_GETFD, 0);
    fcntl(this->sock, F_SETFL, fcntl(this->sock, F_GETFD, 0) & ~O_NONBLOCK);
    auto len = recv(this->sock, reinterpret_cast<void *>(&this->myMessage), sizeof(MessageType), 0);
    fcntl(this->sock, F_SETFL, old_fd_status);

    if (len < 0)
    {
        fprintf(stderr, "Error occurs, %s\n", strerror(errno));
        //this->Close();
        exit(CLIENT_RECV_ERROR);
    }

    switch (this->myMessage.msg_code.Code)
    {
    case CLIENT_ID_NOT_EXIST:
        fprintf(stderr, "Account not exists, please check your account\n");
        //this->Close();
        exit(CLIENT_ID_NOT_EXIST);
    case WRONG_CLIENT_PASSWORD:
        fprintf(stderr, "Account's password error, please check your password\n");
        //this->Close();
        exit(WRONG_CLIENT_PASSWORD);
    case DUPLICATED_LOGIN:
        fprintf(stderr, "Account has been online!\n");
        //this->Close();
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
        //TODO:https://zhuanlan.zhihu.com/p/58708945 父进程死亡子进程同时死亡
        prctl(PR_SET_PDEATHSIG, SIGKILL);

        close(this->pipe_fd[0]);
        while (this->isClientWork)
        {
            //TODO:这里的错误输入处理机制有问题
            bzero(&this->myMessage, sizeof(MessageType));
            //fprintf(stdout, "Message Type Code: \n");
            int selectCode;
            fflush(stdin);
            std::cin >> selectCode;
            if (selectCode > 4 || !selectCode)
            {
                std::cout << "Invalid selection: " << selectCode << std::endl;
                continue;
            }

            switch (selectCode)
            {
            case 4: //Logout
                this->myMessage.OperCode = REQUEST_NORMAL_OFFLINE;
                break;
            case 3: //Get Online List
                this->myMessage.OperCode = REQUEST_ONLINE_LIST;
                break;
            case 2: //Private Message
                this->myMessage.OperCode = PRIVATE_MSG;
                fprintf(stdout, "Send to account: ");
                std::cin >> this->myMessage.msg_code.Whom;

                fprintf(stdout, "给%s的私聊消息: ", this->myMessage.msg_code.Whom);
                getchar();
                std::cin.getline(this->myMessage.msg, BUF_SIZE);

                break;
            case 1: //Group Message
                this->myMessage.OperCode = GROUP_MSG;
                getchar();
                fprintf(stdout, "要发送的群聊消息: ");
                std::cin.getline(this->myMessage.msg, BUF_SIZE);

                break;
            default:
                break;
            }
            if (write(this->pipe_fd[1], reinterpret_cast<void *>(&this->myMessage), sizeof(MessageType)) < 0)
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
                bzero(&this->myMessage, BUF_SIZE);
                if (events[i].data.fd == this->sock) //服务端发来消息
                {
                    if (!recv(this->sock, reinterpret_cast<void *>(&this->myMessage), sizeof(MessageType), 0))
                    {
                        this->isClientWork = false;
                        fprintf(stderr, "Server closed.\n");
                    }
                    else
                    {
                        auto curTime = Client::getTime();
                        switch (this->myMessage.OperCode)
                        {
                        case WELCOME_WITH_IDENTITY_MSG:
                            fprintf(stdout, "[%s] %s\n", curTime.c_str(), this->myMessage.msg);
                            break;
                        case GROUP_MSG:
                            fprintf(stdout, "\033[32m[%s] %s >>> %s\033[0m\n", curTime.c_str(), this->myMessage.msg_code.Whom, this->myMessage.msg);
                            break;
                        case PRIVATE_MSG:
                            fprintf(stdout, "\033[32m[%s] %s(private) >>> %s\033[0m\n", curTime.c_str(), this->myMessage.msg_code.Whom, this->myMessage.msg);
                            break;
                        case REPLY_ONLINE_LIST:
                            //TODO:完善在线列表回复
                            fprintf(stdout, "\033[32m[%s] Accounts Online num: %lu\033[0m\n", curTime.c_str(), this->myMessage.msg_code.online_num);
                            fprintf(stdout, "\033[32m[%s] Accounts Online List: %s\033[0m\n", curTime.c_str(), this->myMessage.msg);
                            break;
                        case ACCEPT_NORMAL_OFFLINE:
                            fprintf(stdout, "\033[32m[%s] Prepared to be offline.\033[0m\n", curTime.c_str());
                            exit(EXIT_SUCCESS);
                            break;
                        default:
                            break;
                        }
                    }
                }
                else //子进程写入事件发生，父进程处理并发送服务端
                {
                    if (read(events[i].data.fd, reinterpret_cast<void *>(&this->myMessage), sizeof(MessageType)) == 0) //子进程死了
                        this->isClientWork = false;
                    else
                        send(this->sock, reinterpret_cast<void *>(&this->myMessage), sizeof(MessageType), 0);
                }
            }
        }
    }
    //this->Close();
}
Client::~Client() { close(this->pipe_fd[!this->pid ? 1 : 0]); }

std::string Client::getTime()
{
    time_t timep;
    time(&timep);
    char ret[24];
    strftime(ret, sizeof(ret), "%Y-%m-%d %H:%M:%S", localtime(&timep));
    return std::string(ret);
}