#pragma once
#include "noncopyable.h"
#include "Timestamp.h"
#include "CurrentThread.h"


#include<functional>
#include<vector>
#include<atomic>
#include<memory>
#include<mutex>

class Channel;
class Poller;

// 事件循环类， 主要包括两个大模块 channel 和 poller
class EventLoop:noncopyable{
public:
    using Functor=std::function<void()>;

    EventLoop();
    ~EventLoop();

    // 开启事件循环
    void loop();
    // 退出事件循环
    void quit();

    Timestamp pollReturnTime()const {return pollReturnTime_;}
    
    // 在当前loop中执行cb
    void runInLoop(Functor cb);
    // 将cb放入队列中，唤醒loop所在的线程，执行cb
    void queueInLoop(Functor cb);

    // 唤醒loop所在线程
    void wakeup();


    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    void hasChannel(Channel* channel);

    // 判断eventloop对象是否在自己的线程里面 
    bool isInLoopThread()const{return threadId_==CurrentThread::tid();}

private:
    void handleRead();
    // 执行回调
    void doPendingFunctors();


    using ChannelList=std::vector<Channel*>;
    
    // 控制事件循环是否正常进行
    std::atomic_bool looping_;
    // 标志退出loop循环
    std::atomic_bool quit_;


    
    // 记录当前loop所在线程的id
    const pid_t threadId_;

    //poller返回发生时间的channels的时间点
    Timestamp pollReturnTime_;
    std::unique_ptr<Poller> poller_;

    // eventfd(),作用：当mainLoop获取一个新用户的channel,通过轮询算法选择一个subloop,通过该成员唤醒subloop处理channel
    int wakeupFd_;
    std::unique_ptr<Channel> wakeupChannel_;

    ChannelList activateChannels_;
    // Channel* currentActivateChannel_;


    // 标识当前loop是否有需要执行的回调操作
    std::atomic_bool callingPendingFunctors_;
    // 存储loop需要执行的回调操作
    std::vector<Functor> pendingFunctors_;
    // 保护pendingFunctors_的线程安全操作
    std::mutex mutex_;
};