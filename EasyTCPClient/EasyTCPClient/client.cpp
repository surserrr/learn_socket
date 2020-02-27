#include "EasyTcpClient.hpp"
#include <thread>
 
void cmdThread(EasyTcpClient* client)
{
    while(true)
    {
        char cmdBuf[256] = {};
        cin>>cmdBuf;
        //cout<<cmdBuf<<"!!!"<<endl;
        if(0==strcmp(cmdBuf, "exit"))
        {
            client->Close();
            cout<<"退出线程"<<endl;
            break;
        }
        else if (0 ==strcmp(cmdBuf, "login"))
        {
            Login login;
            strcpy(login.userName, "fjt");
            strcpy(login.PassWord,"fjtmm");
            client->SendData(&login);
        }
        else if (0 ==strcmp(cmdBuf, "logout"))
        {
            Logout logout;
            strcpy(logout.userName, "fjt");
            client->SendData(&logout);
        }
        else
        {
            cout<<"不支持的命令"<<endl;
        }
    }
}

int main(int argc, const char * argv[])
{
    const int cCount = 10;
    EasyTcpClient *client[cCount];
    for(int n=0;n<cCount;n++)
    {
        client[n] = new EasyTcpClient();
        client[n]->Connect("127.0.0.1", 4567);
    }
    //client.Connect("127.0.0.1", 4567);
    //127.0.0.1 192.168.0.107
    //启动线程
    //std::thread t1(cmdThread);
    //t1.detach();
    
    Login login;
    strcpy(login.userName, "fjt");
    strcpy(login.PassWord, "fjtmm");
    while (true)
    {
        for(int n=0;n<cCount;n++)
        {
            client[n]->SendData(&login);
            client[n]->OnRun();
        }
        //client->OnRun();
        //cout<<"空闲时间处理其他业务 .."<<endl;
        //sleep(1000);
    }
    for(int n=0;n<cCount;n++)
    {
        client[n]->Close();
    }
    cout<<"已退出。\n";
 //   getchar();
    return 0;
}
