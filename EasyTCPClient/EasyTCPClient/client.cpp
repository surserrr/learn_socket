#include "EasyTcpClient.hpp"
#include <thread>

bool g_bRun=true;
void cmdThread()
{
    while (true)
    {
        char cmdBuf[256];
        cin>>cmdBuf;
        if(0 == strcmp(cmdBuf, "exit"))
        {
            //g_bRun = false;
            cout<<"退出cmdthread线程"<<endl;
            break;
        }
        else{
            cout<<"不支持的命令"<<endl;
        }
    }
    /*while(true)
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
    }*/
}

const int cCount = 100;//  客户端数量
const int tCount = 4;// 线程数量
EasyTcpClient *client[cCount];//客户端数组
void sendThread(int id)
{
    //4个线程， id 1-4
    int c = (cCount/tCount);
    int begin = (id-1) * c;
    int end = id*c;
    for(int n=begin;n<end;n++)
    {
        if(!g_bRun){
            return;
        }
        client[n] = new EasyTcpClient();
    }
    for(int n=begin;n<end;n++)
    {
        if(!g_bRun){
            return;
        }
        client[n]->Connect("127.0.0.1", 4567);
        cout<<"Connect="<<n<<endl;
    }

    Login login;
    strcpy(login.userName, "fjt");
    strcpy(login.PassWord, "fjtmm");
    while (g_bRun)
    {
        for(int n=begin;n<end;n++)
        {
            client[n]->SendData(&login);
            //client[n]->OnRun();
        }
    }
    for(int n=begin;n<end;n++)
    {
        client[n]->Close();
    }

}

int main()
{
    signal(SIGPIPE, SIG_IGN);
    //启动ui线程
    std::thread t1(cmdThread);
    t1.detach();
    
    //启动发送线程
    for(int n=0;n<tCount;n++)
    {
        std::thread t1(sendThread,n+1);
        t1.detach();
    }
    while (g_bRun) {
        sleep(1000);
    }
    cout<<"已退出。\n";
    return 0;
}
