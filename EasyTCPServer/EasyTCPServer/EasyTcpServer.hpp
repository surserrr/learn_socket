#ifndef _EasyTcpServer_hpp_
#define _EasyTcpServer_hpp_

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
#include <vector>
#include<iostream>
#include "MessageHeader.hpp"
using namespace std;

class EasyTcpServer
{
private:
    SOCKET _sock;
    std :: vector<SOCKET> g_clients;
 
public:
    EasyTcpServer()
    {
        _sock = INVALID_SOCKET;
    }
    virtual ~EasyTcpServer()
    {
        Close();
    }
    //初始化socket
    SOCKET InitSocket()
    {
#ifdef _WIN32
        // 启动windows socket2.x 环境
        WORD vwe = MAKEWORD(2,2);
        WSADATA dat;
        WSAStartup(ver,&dat);
#endif
        //1.建立一个socket
        if(INVALID_SOCKET != _sock)
        {
            cout<<"<socket="<<(int)_sock<<">关闭旧链接"<<endl;
            Close();
        }
        _sock = socket(AF_INET,SOCK_STREAM, IPPROTO_TCP);
        
        if(INVALID_SOCKET == _sock)
        {
            cout<<"错误！建立套接字失败\n";
        }
        else
        {
            cout<<"建立套接字成功\n";
        }
        return _sock;
    }
    //绑定IP和端口号
    int Bind(const char* ip,unsigned short port)
    {
        sockaddr_in _sin = {};
        _sin.sin_family = AF_INET;
        _sin.sin_port = htons(port); //网络和本机字节序编码不同，需要转换，一个是大端编码，一个是小端编码
#ifdef _WIN32
        if(ip)
        {
            _sin.sin_addr.S_un.s_addr = inet_addr(ip);
        }
        else
        {
            _sin.sin_addr.S_un.s_addr = INADDR_ANY;
        }
#else
        if(ip)
        {
            _sin.sin_addr.s_addr = inet_addr(ip);
        }
        else
        {
            _sin.sin_addr.s_addr = INADDR_ANY;
        }
#endif
        //_sin.sin_addr.s_addr = INADDR_ANY; // 本机ip地址
        int ret = ::bind(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in));
        if(SOCKET_ERROR == ret){
            cout<<"错误!绑定用于接受客户端连接的端口<"<<port<<">失败！"<<endl;
        }
        else{
            cout<<"绑定用于接受客户端连接的端口<"<<port<<">成功！\n";
        }
        return ret;
    }
    //监听端口号
    int Listen(int n)
    {
        int ret = listen(_sock,n);
        if(SOCKET_ERROR == ret){
            cout<<"<socket="<<(int)_sock<<">错误!监听网络端口失败！\n";
        }
        else{
            cout<<"<socket="<<(int)_sock<<">监听网络端口成功！\n";
        }
        return ret;
    }
    //接受客户端连接
    SOCKET Accept()
    {
        sockaddr_in clientAddr = {};
        socklen_t nAddrLen = sizeof(sockaddr_in);
        SOCKET _cSock = INVALID_SOCKET;
#ifdef _WIN32
        _cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
#else
        _cSock = accept(_sock, (sockaddr*)&clientAddr, (socklen_t*)&nAddrLen);
#endif
        if(INVALID_SOCKET == _cSock){
            cout<<"<socket="<<(int)_sock<<">错误!接收到无效客户端SOCKET！\n";
        }
        else
        {
            NewUserJoin userJoin;
            SendDataToAll(&userJoin);
            g_clients.push_back(_cSock);
            cout<<"<socket="<<(int)_sock<<">新客户端加入：socket = "<<(int)_cSock<<"IP="<<inet_ntoa(clientAddr.sin_addr)<<endl;
        }
        return _cSock;
    }
    
    //关闭socket
    void Close()
    {
        if(_sock != INVALID_SOCKET)
        {
#ifdef _WIN32
            for(int n = g_clients.size()-1;n >=0;n--)
            {
                closesocket(g_clients[n]);
            }
            //8. 关闭套接字closesocket
            closesocket(_sock);
#else
            for(int n = (int)g_clients.size()-1;n >=0;n--)
            {
                close(g_clients[n]);
            }
            //8. 关闭套接字closesocket
            close(_sock);
#endif
        }
    }
    //处理网络信息
    bool OnRun()
    {
        if(isRun())
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
                Close();
                return false;
            }
            //判断描述符是否在集合中
            if(FD_ISSET(_sock,&fdRead))
            {
                FD_CLR(_sock,&fdRead);
                Accept();
                //4. accept 等待接受客户端连
            }
            for (int n=(int)g_clients.size()-1; n>=0; n--)
            {
                if(FD_ISSET(g_clients[n], &fdRead))
                {
                    if(-1 == RecvData(g_clients[n]))
                    {
                        auto iter = g_clients.begin() + n;
                        if(iter != g_clients.end())
                        {
                            g_clients.erase(iter);
                        }
                    }
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
        
    //接收数据 处理粘包 拆分包
    int RecvData (SOCKET _cSock)
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
        recv(_cSock, (char *)&szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
        OnNetMsg(_cSock, header);
        return 0;
    }

    //相应网络消息
    virtual void OnNetMsg(SOCKET _cSock, DataHeader* header)
    {
        switch (header->cmd)
        {
            case CMD_LOGIN:
            {
                Login* login = (Login*) header;
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
                Logout* logout = (Logout*) header;
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
    }
    //发送指定数据
    int SendData(SOCKET _cSock,DataHeader* header)
    {
        if(isRun() && header)
        {
            return (int)send(_cSock, (const char*)header, header->dataLength, 0);
        }
        return SOCKET_ERROR;
    }
    //
    void SendDataToAll(DataHeader* header)
    {
       for (int n=(int)g_clients.size()-1; n>=0; n--)
       {
           SendData(g_clients[n],header);
       }
    }
};
#endif
