
//  Created by Surser on 2020/1/17.
//  Copyright © 2020 Surser. All rights reserved.
//
#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include<windows.h>
    #include<WinSock2.h>
    #pragma comment(lib,"ws2_32.lib")
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

#include <vector>
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
std :: vector<SOCKET> g_clients;

int processor (SOCKET _cSock)
{
    //缓冲区
    char szRecv[4096] = {};
    //5. 接受客户端请求
    int nLen = (int)recv(_cSock, (char *)&szRecv, sizeof(DataHeader), 0);
    DataHeader* header = (DataHeader*)szRecv;
    if(nLen<=0){
        cout<<"客户端<Socket="<<_cSock<<">已退出，任务结束。\n";
        return -1;
    }
    switch (header->cmd) {
        case CMD_LOGIN:
        {
            recv(_cSock, (char *)&szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
            Login* login = (Login*) szRecv;
        cout<<"收到客户端<Socket="<<_cSock<<">请求：CMD_LOGIN,数据长度："<<login->dataLength<<",userName="<<login->userName<<",PassWord="<<login->PassWord<<endl;
            //忽略判断用户密码是否正确的过程
            LoginResult ret = {};
            send(_cSock, (char*)&ret, sizeof(LoginResult), 0);
            //char msgBuf[128]= "Xiao Fang.";
            // 8. send 向客户端发送一段数据
            //send(_cSock, msgBuf, strlen(msgBuf)+1, 0);
        }
        break;
        case CMD_LOGOUT:
        {
            Logout* logout = (Logout*) szRecv;
            recv(_cSock, (char *)&szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
            cout<<"收到客户端<Socket="<<_cSock<<">请求：CMD_LOGOUT,数据长度："<<logout->dataLength<<endl;
            //忽略判断用户密码是否正确的过程
            LogoutResult ret = {};
            send(_cSock, (char*)&ret, sizeof(LogoutResult), 0);
        }
        break;
        default:
            DataHeader header = {0,CMD_ERROR};
            send(_cSock, (char*)&header, sizeof(header), 0);
        break;
    }
    return 0;
}

int main(int argc, const char * argv[]) {
#ifdef _WIN32
    // 启动windows socket2.x 环境
    WORD vwe = MAKEWORD(2,2);
    WSADATA dat;
    WSAStartup(ver,&dat);
#endif
    //1. 建立一个socket 三个参数变量，1 ip编码方式 ipv4；2 流式，面向字节流的；3 TCP；
    SOCKET _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    //2. bind 绑定用于接受客户端连接的端口
    sockaddr_in _sin = {};
    _sin.sin_family = AF_INET;
    _sin.sin_port = htons(4567); //网络和本机字节序编码不同，需要转换，一个是大端编码，一个是小端编码
    #ifdef _WIN32
        _sin.sin_addr.S_un.s_addr = INADDR_ANY;
    #else
        _sin.sin_addr.s_addr = INADDR_ANY;
    #endif
    //_sin.sin_addr.s_addr = INADDR_ANY; // 本机ip地址
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
    cout<<"本进程socket="<<_sock<<endl;
    while(true)
    {
        //伯克利套接字 BSD socket 描述符
        fd_set fdRead;//描述符集合
        fd_set fdWrite;
        fd_set fdExp;
        //清理集合
        FD_ZERO(&fdRead);
        FD_ZERO(&fdWrite);
        FD_ZERO(&fdExp);
        //将描述符加入集合
        FD_SET(_sock,&fdRead);
        FD_SET(_sock,&fdWrite);
        FD_SET(_sock,&fdExp);
        SOCKET maxSock = _sock;
        for (int n=(int)g_clients.size()-1; n>=0; n--)
        {
            FD_SET (g_clients[n],&fdRead);
            if(maxSock < g_clients[n]){
                maxSock = g_clients[n];
            }
        }
        ///第一个参数nfds是一个整数，指fd_set集合中所有socket的范围，而不是数量。
        ///即是所有描述符最大值+1，在windows中可以写0.
        timeval t={1,0};
        int ret = select(maxSock+1, &fdRead, &fdWrite, &fdExp, &t); //最后一个参数为NULL，会一直阻塞到有信息进来;若设置一段时间，则会阻塞一段时间。服务器既可以处理多个客户端的请求，也可以在空闲时候做服务器自己的业务处理（例如主动给客户端发信息）。
        if(ret<0)
        {
            cout<<"select任务结束."<<endl;
            break;
        }
        //判断描述符是否在集合中
        if(FD_ISSET(_sock,&fdRead))
        {
            FD_CLR(_sock,&fdRead);
            //4. accept 等待接受客户端连接
            sockaddr_in clientAddr = {};
            socklen_t nAddrLen = sizeof(sockaddr_in);
            SOCKET _cSock = INVALID_SOCKET;
#ifdef _WIN32
            _cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
#else
            _cSock = accept(_sock, (sockaddr*)&clientAddr, (socklen_t*)&nAddrLen);
#endif
            if(INVALID_SOCKET == _cSock){
                cout<<"ERROR!接收到无效客户端SOCKET！\n";
            }
            else
            {
                for (int n=(int)g_clients.size()-1; n>=0; n--)
                {
                    NewUserJoin userJoin;
                    send(g_clients[n], (char *)&userJoin, sizeof(NewUserJoin), 0);
                }
                    g_clients.push_back(_cSock);
                    cout<<"新客户端加入：socket = "<<(int)_cSock<<"IP="<<inet_ntoa(clientAddr.sin_addr)<<endl;
            }
           
        }
        
        for (int n=(int)g_clients.size()-1; n>=0; n--)
        {
            if(FD_ISSET(g_clients[n], &fdRead))
            {
                if(-1 == processor(g_clients[n]))
                {
                    auto iter = g_clients.begin() + n;
                    if(iter != g_clients.end())
                    {
                        g_clients.erase(iter);
                    }
                }
            }
        }

        //cout<<"空闲时间处理其他业务 .."<<endl;
    }
    
#ifdef _WIN32
    for(int n = g_clients.size()-1;n >=0;n--)
    {
        closesocket(g_clients[n]);
    }
    //8. 关闭套接字closesocket
    closesocket(_sock);
#else
    for(int n = g_clients.size()-1;n >=0;n--)
    {
        close(g_clients[n]);
    }
    //8. 关闭套接字closesocket
    close(_sock);
#endif
//
    cout<<"服务器已退出。\n";
    getchar();
    return 0;
}
