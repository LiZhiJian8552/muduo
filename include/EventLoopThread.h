#pragma once
#include"noncopyable.h"
#include "EventLoop.h"
#include"Thread.h"

#include<functional>
#include<mutex>
#include<condition_variable>



// 结合Loop和Thread
class EventLoopThread:noncopyable{
public:
    using ThreadInitCallback=std::function<void(EventLoop*)>;

    EventLoopThread(const ThreadInitCallback& cb=ThreadInitCallback(),const std::string& name=std::string());
    ~EventLoopThread();
    EventLoop* startLoop();

private:
    // 创建线程的函数
    void threadFunc();

    EventLoop* loop_;
    bool exiting_;
    Thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;    

    ThreadInitCallback callback_;
};