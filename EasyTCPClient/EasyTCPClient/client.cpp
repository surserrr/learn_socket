//
//  main.cpp
//  hellocpp
//
//  Created by Surser on 2020/1/17.
//  Copyright © 2020 Surser. All rights reserved.
//
#ifdef _WIN32
    #include<windows.h>
    #include<WinSock2.h>
#else
    #include <unistd.h>
    #include<arpa/inet.h>
    #include<string.h>

    #define SOCKET int
    #define INVALID_SOCKET (SOCKET) (~0)
    #define SOCKET_ERROR (-1)
#endif

#include <stdio.h>
#include<thread>
#include<iostream>
using namespace std;

enum CMD
{
    CMD_LOGIN,
    CMD_LOGIN_RESULT,
    CMD_LOGOUT,
    CMD_LOGOUT_RESULT,
    CMD_NEW_USER_JOIN,
    CMD_ERROR
};

struct DataHeader
{
    short dataLength;
    short cmd;
};
//DataPacket
struct Login:public DataHeader
{
    Login()
    {
        dataLength = sizeof(Login);
        cmd = CMD_LOGIN;
    }
    char userName[32];
    char PassWord[32];
};
struct LoginResult:public DataHeader
{
    LoginResult()
    {
        dataLength = sizeof(LoginResult);
        cmd = CMD_LOGIN_RESULT;
        result = 0;
    }
    int result;
};
struct Logout:public DataHeader
{
    Logout()
    {
        dataLength = sizeof(Logout);
        cmd = CMD_LOGOUT;
    }
    char userName[32];
};
struct LogoutResult:public DataHeader
{
    LogoutResult()
    {
        dataLength = sizeof(LogoutResult);
        cmd = CMD_LOGOUT_RESULT;
        result = 0;
    }
    int result;
};

struct NewUserJoin:public DataHeader
{
    NewUserJoin()
    {
        dataLength = sizeof(NewUserJoin);
        cmd = CMD_NEW_USER_JOIN;
        scok = 0;
    }
    int scok;
};

int processor (SOCKET _cSock)
{
    //缓冲区
    char szRecv[4096] = {};
    //5. 接受客户端请求
    int nLen = (int)recv(_cSock, (char *)&szRecv, sizeof(DataHeader), 0);
    DataHeader* header = (DataHeader*)szRecv;
    if(nLen<=0){
        cout<<"与服务器断开连接，任务结束"<<endl;
        return -1;
    }
    switch (header->cmd) {
        case CMD_LOGIN_RESULT:
        {
            recv(_cSock, (char *)&szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
            LoginResult* login = (LoginResult*) szRecv;
            cout<<"收到服务器消息：CMD_LOGIN_RESULT,数据长度："<<login->dataLength<<endl;
        }
        break;
        case CMD_LOGOUT_RESULT:
        {
            recv(_cSock, (char *)&szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
            LogoutResult* logout = (LogoutResult*) szRecv;
            cout<<"收到服务器消息：CMD_LOGOUT_RESULT,数据长度："<<logout->dataLength<<endl;
        }
        break;
        case CMD_NEW_USER_JOIN:
        {
            recv(_cSock, (char *)&szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
            NewUserJoin* userJoin = (NewUserJoin*) szRecv;
            cout<<"收到服务器消息：CMD_NEW_USER_JOIN,数据长度："<<userJoin->dataLength<<endl;
        }
        break;
        default:
            cout<<"test"<<endl;
        break;
    }
    return 0;
}
bool g_bRun = true;
void cmdThread(SOCKET sock)
{
    while(g_bRun)
    {
        char cmdBuf[256] = {};
        cin>>cmdBuf;
        if(0==strcmp(cmdBuf, "exit"))
        {
            g_bRun = false;
            cout<<"退出线程"<<endl;
            break;
        }
        else if (0 ==strcmp(cmdBuf, "login"))
        {
            Login login;
            strcpy(login.userName, "fjt");
            strcpy(login.PassWord,"fjtmm");
            send(sock, (const char*)&login, sizeof(Login), 0);
        }
        else if (0 ==strcmp(cmdBuf, "logout"))
        {
            Logout logout;
            strcpy(logout.userName, "fjt");
            send(sock, (const char*)&logout, sizeof(Logout), 0);
        }
        else
        {
            cout<<"不支持的命令"<<endl;
        }
    }
}

int main(int argc, const char * argv[]) {
#ifdef _WIN32
    // 启动windows socket2.x 环境
    WORD vwe = MAKEWORD(2,2);
    WSADATA dat;
    WSAStartup(ver,&dat);
#endif
    
//    1.建立一个socket
    SOCKET _sock = socket(AF_INET, SOCK_STREAM, 0);
    if(INVALID_SOCKET == _sock){
        cout<<"错误！建立套接字失败\n";
    }
    else{
        cout<<"建立套接字成功\n";
    }
//    2.连接服务器connect
    sockaddr_in _sin = {};
    _sin.sin_family = AF_INET;
    _sin.sin_port = htons(4567);
#ifdef _WIN32
    _sin.sin_addr.S_un.s_addr = inet_addr("127.0.0.1");
#else
    _sin.sin_addr.s_addr = inet_addr("127.0.0.1");
#endif
    int ret = connect(_sock, (sockaddr *) &_sin, sizeof(sockaddr_in));
    if(SOCKET_ERROR == ret){
        cout<<"错误！连接服务器失败\n";
    }
    else{
        cout<<"连接服务器成功\n";
    }
    //启动线程
    std::thread t1(cmdThread,_sock);
    t1.detach();
    
    while (g_bRun)
    {
        fd_set fdReads;
        FD_ZERO(&fdReads);
        FD_SET(_sock,&fdReads);
        timeval t={1,0};
        int ret = select(_sock+1, &fdReads, NULL, NULL, &t);
        if(ret<0){
            cout<<"select任务结束1"<<endl;
            break;
        }
        if(FD_ISSET(_sock, &fdReads)){
            FD_CLR(_sock, &fdReads);
            if(-1 == processor(_sock))
            {
                cout<<"select任务结束2"<<endl;
                break;
            }
        }
        //cout<<"空闲时间处理其他业务 .."<<endl;
        //sleep(1000);
    }
//    7.关闭套接字close
#ifdef _WIN32
    closesocket(_sock);
#else
    close(_sock);
#endif
    cout<<"客户端已退出。\n";
 //   getchar();
    return 0;
}
