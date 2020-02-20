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
    
    char _recvbuf[128]={};
    while(true){
        //5. 接受客户端请求
        int nLen = recv(_cSock, _recvbuf, 128, 0);
        if(nLen<=0){
            cout<<"客户端已退出，任务结束。\n";
            break;
        }
        cout<<"收到命令："<<_recvbuf<<"\n";
        //6. 处理请求
        if(0 == strcmp(_recvbuf, "getName")){
            char msgBuf[128]= "Xiao Fang.";
            // 8. send 向客户端发送一段数据
            send(_cSock, msgBuf, strlen(msgBuf)+1, 0);
        }else if(0 == strcmp(_recvbuf, "getAge")){
            char msgBuf[128]= "18 years old.";
            send(_cSock, msgBuf, strlen(msgBuf)+1, 0);
        }else {
            char msgBuf[128]= "???.";
            send(_cSock, msgBuf, strlen(msgBuf)+1, 0);
        }
    }
    //8. 关闭套接字closesocket
    close(_sock);
//
    cout<<"服务器已退出。\n";
    getchar();
    return 0;
}
