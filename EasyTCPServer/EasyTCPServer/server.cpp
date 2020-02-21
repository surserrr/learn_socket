
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

int main(int argc, const char * argv[]) {
//
    //1. 建立一个socket 三个参数变量，1 ip编码方式 ipv4；2 流式，面向字节流的；3 TCP；
    SOCKET _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    //2. bind 绑定用于接受客户端连接的端口
    sockaddr_in _sin = {};
    _sin.sin_family = AF_INET;
    _sin.sin_port = htons(4567); //网络和本机字节序编码不同，需要转换，一个是大端编码，一个是小端编码
    _sin.sin_addr.s_addr = INADDR_ANY; // 本机ip地址
    if(SOCKET_ERROR == ::bind(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in))){
        cout<<"ERROR!绑定用于接受客户端连接的端口失败！\n";
    }
    else{
        cout<<"绑定用于接受客户端连接的端口成功！\n";
    }
    
    //3. listen 监听网络端口
    if(SOCKET_ERROR == listen(_sock, 5)){
        cout<<"ERROR!监听网络端口失败！\n";
    }
    else{
        cout<<"监听网络端口成功！\n";
    }
    //4. accept 等待接受客户端连接
    sockaddr_in clientAddr = {};
    socklen_t nAddrLen = sizeof(sockaddr_in);
    SOCKET _cSock = INVALID_SOCKET;
    
    _cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
    if(INVALID_SOCKET == _cSock){
        cout<<"ERROR!接收到无效客户端SOCKET！\n";
    }
    cout<<"新客户端加入：socket = "<<(int)_cSock<<"IP="<< inet_ntoa(clientAddr.sin_addr)<<"\n";
    
    while(true)
    {
        DataHeader header = {};
        //5. 接受客户端请求
        int nLen = recv(_cSock, (char *)&header, sizeof(header), 0);
        if(nLen<=0){
            cout<<"客户端已退出，任务结束。\n";
            break;
        }
        switch (header.cmd) {
            case CMD_LOGIN:
            {
                Login login{};
                recv(_cSock, (char *)&login + sizeof(DataHeader), sizeof(Login) - sizeof(DataHeader), 0);
                cout<<"收到命令：CMD_LOGIN,数据长度："<<login.dataLength
                <<",userName="<<login.userName<<",PassWord="<<login.PassWord;
                //忽略判断用户密码是否正确的过程
                LoginResult ret;
                send(_cSock, (char*)&ret, sizeof(ret), 0);
            }
            break;
            case CMD_LOGOUT:
            {
                Logout logout;
                recv(_cSock, (char *)&logout + sizeof(DataHeader), sizeof(logout) - sizeof(DataHeader), 0);
                cout<<"收到命令：CMD_LOGOUT,数据长度："<<logout.dataLength;
                //忽略判断用户密码是否正确的过程
                LogoutResult ret = {};
                send(_cSock, (char*)&ret, sizeof(ret), 0);
            }
            break;
            default:
                header.cmd = CMD_ERROR;
                header.dataLength = 0;
                send(_cSock, (char*)&header, sizeof(header), 0);
            break;
        }
    }
    //8. 关闭套接字closesocket
    close(_sock);
//
    cout<<"服务器已退出。\n";
    getchar();
    return 0;
}
