#ifndef _EasyTcpServer_hpp_
#define _EasyTcpServer_hpp_
#define _WINSOCK_DEPRECATED_NO_WARNINGS 
#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
	#define FD_SETSIZE 1024
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


//#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <vector>
#include <thread>
#include <iostream>
#include <mutex>
#include <atomic>
#include "MessageHeader.hpp"
#include "CELLTimestamp.hpp"
using namespace std;

#ifndef RECV_BUFF_SIZE
#define RECV_BUFF_SIZE 10240
#endif
#define _CellServer_THREAD_COUNT 2
class ClientSocket
{
public:
    ClientSocket(SOCKET sockfd = INVALID_SOCKET)
    {
        _sockfd = sockfd;
        memset(_szMsgBuf,0,sizeof(_szMsgBuf));
        _lastPos=0;
    }
    
    SOCKET sockfd()
    {
        return _sockfd;
    }
    
    char* msgBuf()
    {
        return _szMsgBuf;
    }
    
    int getLastPos()
    {
        return _lastPos;
    }
    void setLastPos(int pos)
    {
        _lastPos = pos;
    }
private:
    SOCKET _sockfd;//fd_set file desc set
    //第二缓冲区 消息缓冲区
    char _szMsgBuf[RECV_BUFF_SIZE*10];
    int _lastPos = 0;
    
};

class CellServer
{
public:
    CellServer(SOCKET sock=INVALID_SOCKET)
    {
        _sock = sock;
        _pThread = nullptr;
        _recvCount = 0;
    }
    ~CellServer()
    {
        Close();
        _sock = INVALID_SOCKET;
        
    }
    void Close()
    {
        if(_sock != INVALID_SOCKET)
        {
#ifdef _WIN32
            for(int n = (int)_clients.size()-1;n >=0;n--)
            {
                closesocket(_clients[n]->sockfd());
                delete _clients[n];
            }
            //8. 关闭套接字closesocket
            closesocket(_sock);
#else
            for(int n = (int)_clients.size()-1;n >=0;n--)
            {
                close(_clients[n]->sockfd());
                delete _clients[n];
            }
            //8. 关闭套接字closesocket
            close(_sock);
#endif
            _clients.clear();
        }
    }
    
    bool isRun()
    {
        return _sock!=INVALID_SOCKET;
    }
    
    bool OnRun()
    {
        while(isRun())
        {
            if(_clientsBuff.size()>0)
            {
                std::lock_guard<std::mutex> lock(_mutex);
                for(auto pClient : _clientsBuff)
                {
                    _clients.push_back(pClient);
                }
                _clientsBuff.clear();
            }
            //如果没有需要处理的客户端，就跳过。
            if(_clients.empty())
            {
                std::chrono::milliseconds t(1); //休眠1ms
                std::this_thread::sleep_for(t);
                continue;
            }
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
            SOCKET maxSock = _clients[0]->sockfd();
            for (int n=(int)_clients.size()-1; n>=0; n--)
            {
                FD_SET (_clients[n]->sockfd(),&fdRead);
                if(maxSock < _clients[n]->sockfd()){
                    maxSock = _clients[n]->sockfd();
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

            for (int n=(int)_clients.size()-1; n>=0; n--)
            {
                if(FD_ISSET(_clients[n]->sockfd(), &fdRead))
                {
                    if(-1 == RecvData(_clients[n]))
                    {
                        auto iter = _clients.begin() + n;
                        if(iter != _clients.end())
                        {
                            delete _clients[n];
                            _clients.erase(iter);
                        }
                    }
                }
            }
        }
        return true;
    }
    
    char szRecv[RECV_BUFF_SIZE];
    //接收数据 处理粘包 拆分包
    int RecvData (ClientSocket* pClient)
    {
        //5. 接受客户端请求
        int nLen = (int)recv(pClient->sockfd(), (char *)&szRecv, RECV_BUFF_SIZE, 0);
        
        if(nLen<=0){
            cout<<"客户端<Socket="<<pClient->sockfd()<<">已退出，任务结束。\n"<<endl;
            return -1;
        }
        //将收到的数据拷贝到消息缓冲区
        memcpy(pClient->msgBuf() + pClient->getLastPos(), szRecv, nLen);
        pClient->setLastPos(pClient->getLastPos()+nLen);
        
        //判断消息缓冲区的数据是否大于DataHeader长度，
        while(pClient->getLastPos() >= sizeof(DataHeader))
        {
            //这时就可以知道当前消息的长度
            DataHeader* header = (DataHeader*)pClient->msgBuf();
            //判断消息缓冲区的数据是否大于消息 长度，
            if(pClient->getLastPos() >= header->dataLength)
            {
                //消息缓冲区剩余未处理数据的长度
                int nSize = pClient->getLastPos() - header->dataLength;
                //处理网络消息
                OnNetMsg(pClient->sockfd(),header);
                //将消息缓冲区剩余未处理数据前移
                memcpy(pClient->msgBuf(), pClient->msgBuf()+header->dataLength, nSize);
                //尾部位置前移
                pClient->setLastPos(nSize);
            }
            else
            {
                //消息缓冲区剩余数据不够一条完整消息
                break;
            }
        }
        return 0;
    }
    virtual void OnNetMsg(SOCKET cSock, DataHeader* header)
    {
        _recvCount++;
        switch (header->cmd)
        {
            case CMD_LOGIN:
            {
                //Login* login = (Login*) header;
            //cout<<"收到客户端<Socket="<<cSock<<">请求：CMD_LOGIN,数据长度："<<login->dataLength<<",userName="<<login->userName<<",PassWord="<<login->PassWord<<endl;
                //忽略判断用户密码是否正确的过程
                //LoginResult ret;
                //SendData(cSock, &ret);
                //char msgBuf[128]= "Xiao Fang.";
                // 8. send 向客户端发送一段数据
                //send(_cSock, msgBuf, strlen(msgBuf)+1, 0);
            }
            break;
            case CMD_LOGOUT:
            {
                //Logout* logout = (Logout*) header;
                //cout<<"收到客户端<Socket="<<cSock<<">请求：CMD_LOGOUT,数据长度："<<logout->dataLength<<endl;
                //忽略判断用户密码是否正确的过程
                //LogoutResult ret;
                //SendData(cSock, &ret);
            }
            break;
            default:
            {
                cout<<"收到未定义的消息数据，数据长度："<<header->dataLength<<endl;
                //DataHeader header = {0,CMD_ERROR};
                //SendData(_cSock, &ret);
            }
            break;
        }
    }
    
    void addClient(ClientSocket* pClient)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _clientsBuff.push_back(pClient);
    }
    
    void Start()
    {
        //std::mem_fun();
        _pThread = new std::thread(std::mem_fun(&CellServer::OnRun),this);
    }
    size_t getClientCount()
    {
        return _clients.size()+_clientsBuff.size();
    }
private:
    SOCKET _sock;

    //正式客户队列
    std :: vector<ClientSocket*> _clients;
    //缓冲客户队列
    std :: vector<ClientSocket*> _clientsBuff;
    std :: mutex _mutex;
    std :: thread* _pThread;
public:
    std :: atomic_int _recvCount;
};
//new 堆内存；
class EasyTcpServer
{
private:
    SOCKET _sock;
    std :: vector<ClientSocket*> _clients;
    std :: vector<CellServer*> _cellServers;
    CELLTimestamp _tTime;
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
        WSAStartup(vwe,&dat);
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
            _sin.sin_addr.S_un.S_addr = inet_addr(ip);
        }
        else
        {
            _sin.sin_addr.S_un.S_addr = INADDR_ANY;
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
        int nAddrLen = sizeof(sockaddr_in);
        SOCKET cSock = INVALID_SOCKET;
#ifdef _WIN32
        cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
#else
        cSock = accept(_sock, (sockaddr*)&clientAddr, (socklen_t*)&nAddrLen);
#endif
        if(INVALID_SOCKET == cSock){
            cout<<"<socket="<<(int)_sock<<">错误!接收到无效客户端SOCKET！\n";
        }
        else
        {
            //NewUserJoin userJoin;
            //SendDataToAll(&userJoin);
            addClientToCellServer(new ClientSocket(cSock));
            //cout<<"<socket="<<(int)_sock<<">新客户端<"<<_clients.size()<<">加入：socket = "<<(int)cSock<<"IP="<<inet_ntoa(clientAddr.sin_addr)<<endl;
        }
        return cSock;
    }
    void addClientToCellServer(ClientSocket* pClient)
    {
        _clients.push_back(pClient);
        //查找客户数量最少的cellsever消息处理线程对象
        auto pMinServer =_cellServers[0];
        for(auto pCellServer : _cellServers)
        {
            if(pMinServer->getClientCount() > pCellServer->getClientCount())
            {
                pMinServer = pCellServer;
            }
        }
        pMinServer->addClient(pClient);
    }
    void Start()
    {
        for(int n = 0; n<_CellServer_THREAD_COUNT;n++)
        {
            auto ser = new CellServer(_sock);
            _cellServers.push_back(ser);
            ser->Start();            
        }
    }
    
    //关闭socket
    void Close()
    {
        if(_sock != INVALID_SOCKET)
        {
#ifdef _WIN32
            for(int n = (int)_clients.size()-1;n >=0;n--)
            {
                closesocket(_clients[n]->sockfd());
                delete _clients[n];
            }
            //8. 关闭套接字closesocket
            closesocket(_sock);
#else
            for(int n = (int)_clients.size()-1;n >=0;n--)
            {
                close(_clients[n]->sockfd());
                delete _clients[n];
            }
            //8. 关闭套接字closesocket
            close(_sock);
#endif
            _clients.clear();
        }
    }
    //处理网络信息
    bool OnRun()
    {
        if(isRun())
        {
            time4msg();
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
            ///第一个参数nfds是一个整数，指fd_set集合中所有socket的范围，而不是数量。
            ///即是所有描述符最大值+1，在windows中可以写0.
            timeval t={1,0};
            int ret = select(_sock+1, &fdRead, &fdWrite, &fdExp, &t); //最后一个参数为NULL，会一直阻塞到有信息进来;若设置一段时间，则会阻塞一段时间。服务器既可以处理多个客户端的请求，也可以在空闲时候做服务器自己的业务处理（例如主动给客户端发信息）。
            if(ret<0)
            {
                cout<<"select 任务结束."<<endl;
                Close();
                return false;
            }
            //判断描述符是否在集合中
            if(FD_ISSET(_sock,&fdRead))
            {
                FD_CLR(_sock,&fdRead);
                Accept();
                return true;
                //4. accept 等待接受客户端连
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
    
    

    //相应网络消息
    void time4msg()
    {
        auto t1 = _tTime.getElapsedSecond();
        if (t1 >= 1.0) {
            int recvCount = 0;
            for(auto ser: _cellServers)
            {
                recvCount += ser -> _recvCount;
                ser->_recvCount = 0;
            }
            cout<<"time<"<<t1<<">,socket<"<<_sock<<">,clients"<<_clients.size()<<">,recvCount"<<recvCount<<">"<<endl;
            recvCount = 0;
            _tTime.update();
        }
    }
    //发送指定数据
    int SendData(SOCKET cSock,DataHeader* header)
    {
        if(isRun() && header)
        {
            return (int)send(cSock, (const char*)header, header->dataLength, 0);
        }
        return SOCKET_ERROR;
    }
    //
    void SendDataToAll(DataHeader* header)
    {
       for (int n=(int)_clients.size()-1; n>=0; n--)
       {
           SendData(_clients[n]->sockfd(),header);
       }
    }
};
#endif
