#ifndef _EasyTcpClient_hpp_
#define _EasyTcpClient_hpp_

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
#include<iostream>
#include "MessageHeader.hpp"
using namespace std;

class EasyTcpClient
{
    SOCKET _sock;
public:
    EasyTcpClient()
    {
        _sock = INVALID_SOCKET;
        //
    }
    
    virtual ~EasyTcpClient()
    {
        Close();
    }
    //初始化socket
    void InitSocket()
    {
#ifdef _WIN32
        // 启动windows socket2.x 环境
        WORD vwe = MAKEWORD(2,2);
        WSADATA dat;
        WSAStartup(ver,&dat);
#endif
    //    1.建立一个socket
        if(INVALID_SOCKET != _sock)
        {
            cout<<"<socket="<<_sock<<">关闭旧链接"<<endl;
            Close();
        }
        _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        
        if(INVALID_SOCKET == _sock)
        {
            cout<<"错误！建立套接字失败\n";
        }
        else
        {
            cout<<"建立套接字成功\n";
        }
    }
    //连接服务器
    int Connect(char * ip, unsigned short port)
    {
        if(INVALID_SOCKET == _sock)
        {
            InitSocket();
        }
        //    2.连接服务器connect
        sockaddr_in _sin = {};
        _sin.sin_family = AF_INET;
        _sin.sin_port = htons(port);
#ifdef _WIN32
        _sin.sin_addr.S_un.s_addr = inet_addr(ip);
#else
        _sin.sin_addr.s_addr = inet_addr(ip);
#endif
        int ret = connect(_sock, (sockaddr *) &_sin, sizeof(sockaddr_in));
        if(SOCKET_ERROR == ret){
                    cout<<"错误！连接服务器失败\n";
                }
        else{
                    cout<<"连接服务器成功\n";
                }
        return ret;
    }
    //关闭socket
    void Close()
    {
        if(_sock != INVALID_SOCKET)
        {
#ifdef _WIN32
            closesocket(_sock);
#else
            close(_sock);
#endif
            _sock = INVALID_SOCKET;
        }
    }
    //处理数据
    bool OnRun()
    {
        if(isRun())
        {
            fd_set fdReads;
            FD_ZERO(&fdReads);
            FD_SET(_sock,&fdReads);
            timeval t={1,0};
            int ret = select(_sock+1, &fdReads, NULL, NULL, &t);
            if(ret<0){
                cout<<"<socket="<<_sock<<">select任务结束1"<<endl;
                Close();
                return false;
            }
            if(FD_ISSET(_sock, &fdReads)){
                FD_CLR(_sock, &fdReads);
                if(-1 == RecvData(_sock))
                {
                cout<<"<socket="<<_sock<<">select任务结束2"<<endl;
                    Close();
                    return false;
                }
            }
            return true;
        }
        return false;
    }
    //是否工作中
    bool isRun()
    {
        return _sock!=INVALID_SOCKET;
    }
    //接收数据,处理粘包 拆分包
    int RecvData (SOCKET _cSock)
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
        recv(_cSock, (char *)&szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
        OnNetMsg(header);
        return 0;
    }
    //响应网络消息
    virtual void OnNetMsg(DataHeader* header)
    {
        
        switch (header->cmd) {
            case CMD_LOGIN_RESULT:
            {
                LoginResult* login = (LoginResult*) header;
                cout<<"收到服务器消息：CMD_LOGIN_RESULT,数据长度："<<login->dataLength<<endl;
            }
            break;
            case CMD_LOGOUT_RESULT:
            {
                LogoutResult* logout = (LogoutResult*) header;
                cout<<"收到服务器消息：CMD_LOGOUT_RESULT,数据长度："<<logout->dataLength<<endl;
            }
            break;
            case CMD_NEW_USER_JOIN:
            {
                NewUserJoin* userJoin = (NewUserJoin*) header;
                cout<<"收到服务器消息：CMD_NEW_USER_JOIN,数据长度："<<userJoin->dataLength<<endl;
            }
            break;
            default:
                cout<<"test"<<endl;
            break;
        }
    }
    //发送数据
    int SendData(DataHeader* header)
    {
        if(isRun() && header)
        {
            return (int)send(_sock, (const char*)header, header->dataLength, 0);
        }
        return SOCKET_ERROR;
    }
};
#endif
