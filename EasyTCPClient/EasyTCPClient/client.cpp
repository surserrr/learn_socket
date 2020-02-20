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
        else
        {
            //5.向服务器发送请求命令
            send(_sock, cmdBuf, strlen(cmdBuf) +1, 0);
            cout<<"发送成功\n";
        }
        char recvBuf[128] = {};
        int nlen = recv(_sock, recvBuf, 128, 0);
        if(nlen>0){
            cout<<"接收到数据:"<<recvBuf<<"\n";
        }
        else{
            cout<<"没接收到数据\n";
        }
    }
    // 6.接受服务器信息recv
    
//    7.关闭套接字close
    close(_sock);
    cout<<"客户端已退出。\n";
    getchar();
    return 0;
}
