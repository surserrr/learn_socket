#include "EasyTcpServer.hpp"
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

bool g_bRun = true;
void cmdThread()
{
    while (g_bRun)
    {
        char cmdBuf[256];
        cin>>cmdBuf;
        if(0 == strcmp(cmdBuf, "exit"))
        {
            g_bRun = false;
            cout<<"退出cmdthread线程"<<endl;
            break;
        }
        else{
            cout<<"不支持的命令"<<endl;
        }
    }
}

class MyServer : public EasyTcpServer
{
public:
    //只会被一个线程调用 安全
    virtual void OnNetJoin(ClientSocket* pClient)
    {
        _clientCount++;
        cout<<"client<"<<pClient->sockfd()<<"> join."<<endl;
    }
    //cellServer 4 多个线程触发 不安全
    virtual void OnNetLeave(ClientSocket* pClient)
    {
        _clientCount--;
        cout<<"client<"<<pClient->sockfd()<<"> leave."<<endl;
    }
     //cellServer 4 多个线程触发 不安全
    virtual void OnNetMsg(ClientSocket* pClient, DataHeader* header)
    {
        _recvCount++;
        switch (header->cmd)
        {
            case CMD_LOGIN:
            {
                Login* login = (Login*) header;
            //cout<<"收到客户端<Socket="<<cSock<<">请求：CMD_LOGIN,数据长度："<<login->dataLength<<",userName="<<login->userName<<",PassWord="<<login->PassWord<<endl;
                //忽略判断用户密码是否正确的过程
                LoginResult ret;
                pClient->SendData(&ret);
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
                cout<<"<socket="<<pClient->sockfd()<<">收到未定义的消息数据，数据长度："<<header->dataLength<<endl;
                //DataHeader header = {0,CMD_ERROR};
                //SendData(_cSock, &ret);
            }
            break;
        }
    }
};

int main()
{
    MyServer server;
    server.InitSocket();
    server.Bind(nullptr, 4567);
    server.Listen(5);
    server.Start(2);//输入多少个线程
    
    std::thread t1(cmdThread);
    t1.detach();
    while(server.isRun())
    {
        if(!g_bRun){
            break;
        }
        server.OnRun();
        //cout<<"空闲时间处理其他业务 .."<<endl;
    }
    
    server.Close();
    cout<<"服务器已退出。\n";
    getchar();
    return 0;
    
}
