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
    CMD_LOGOUT,
    CMD_ERROR
};

struct DataHeader
{
    short dataLength;
    short cmd;
};
//DataPacket
struct Login
{
    char userName[32];
    char PassWord[32];
};
struct LoginResult
{
    int result;
};
struct Logout
{
    char userName[32];
};
struct LogoutResult{
    int result;
};

int main(int argc, const char * argv[]) {
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
    _sin.sin_addr.s_addr = inet_addr("127.0.0.1");
    int ret = connect(_sock, (sockaddr *) &_sin, sizeof(sockaddr_in));
    if(SOCKET_ERROR == ret){
        cout<<"错误！连接服务器失败\n";
    }
    else{
        cout<<"连接服务器成功\n";
    }
    
    while (true)
    {
        // 3.输入请求命令
        char cmdBuf[128]={};
        cin>>cmdBuf;
        // 4.处理请求命令
        if(0 == strcmp(cmdBuf, "exit"))
        {
            cout<<"收到exit，任务结束。\n";
            break;
        }
        else if( 0 == strcmp(cmdBuf, "login"))
        {
            Login login = {"fjt","fjtmm"};
            DataHeader dh = {sizeof(login), CMD_LOGIN};
            //5.向服务器发送请求命令
            send(_sock, (const char*)&dh, sizeof(dh), 0);
            send(_sock, (const char*)&login, sizeof(login), 0);
            // 接受服务器返回的数据
            DataHeader retHeader = {};
            LoginResult loginRet = {};
            recv(_sock, (char*) &retHeader, sizeof(retHeader), 0);
            recv(_sock, (char*) &loginRet, sizeof(loginRet), 0);
            cout<<"LoginResult:"<<loginRet.result;
        }
        else if( 0 == strcmp(cmdBuf, "logout"))
        {
            Logout logout = {"fjt"};
            DataHeader dh = {sizeof(logout), CMD_LOGOUT};
            send(_sock, (const char*)&dh, sizeof(dh), 0);
            send(_sock, (const char*)&logout, sizeof(logout), 0);
            // 接受服务器返回的数据
            DataHeader retHeader = {};
            LoginResult logoutRet = {};
            recv(_sock, (char*) &retHeader, sizeof(retHeader), 0);
            recv(_sock, (char*) &logoutRet, sizeof(logoutRet), 0);
            cout<<"LogoutRet:"<<logoutRet.result;
        }
        else {
            cout<< "不支持命令，请重新输入\n";
        }
    }
//    7.关闭套接字close
    close(_sock);
    cout<<"客户端已退出。\n";
    getchar();
    return 0;
}
