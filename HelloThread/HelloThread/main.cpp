//
//  main.cpp
//  HelloThread
//
//  Created by Surser on 2020/2/27.
//  Copyright © 2020 Surser. All rights reserved.
//

#include <iostream>
#include <thread>
#include <mutex> //锁
#include<atomic>//原子操作 必须要完成这个操作才可以继续下去
using namespace std;

const int tCount =4;
mutex m;
// atomic<int> s(0);
atomic_int s(0);
  
void workFun(int index)
{
    for(int n=0;n<40;n++)
    {
        //m.lock();//临界区域
        //cout<<index<<"hello thread."<<n<<endl;
        //m.unlock();
        s++;
    }
}
int main()
{
    thread t[tCount];
    for(int n=0;n<tCount;n++)
    {
        t[n] = thread(workFun,n);
    }
    for(int n=0;n<tCount;n++)
    {
        t[n].join();
        //t[n].detach();
    }
    //t.detach();
    //t.join();
    cout<<s<<endl;
    for(int n=0;n<4;n++)
        cout<<"Hello,main thread."<<endl;
    return 0;
}
