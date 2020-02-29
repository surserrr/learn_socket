#include "EasyTcpServer.hpp"
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS 
int main()
{
    EasyTcpServer server;
    server.InitSocket();
    server.Bind(nullptr, 4567);
    server.Listen(5);
    server.Start();
    while(server.isRun())
    {
        server.OnRun();
        //cout<<"空闲时间处理其他业务 .."<<endl;
    }
    server.Close();
    cout<<"服务器已退出。\n";
    getchar();
    return 0;
    
}
