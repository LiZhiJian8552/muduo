#include"EventLoopThread.h"
#include "EventLoopThread.h"

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb, const std::string &name)
    :loop_(nullptr),
    exiting_(false),
    thread_(std::bind(&EventLoopThread::threadFunc,this),name),
    mutex_(),   //默认构造
    cond_(),    //默认构造
    callback_(cb)
{

}

EventLoopThread::~EventLoopThread(){
    exiting_=true;
    // 如果loop不为空，则退出
    if(loop_!=nullptr){
        loop_->quit();
        // 等待子线程结束
        thread_.join();
    }

}

// 启动循环
EventLoop *EventLoopThread::startLoop(){
    thread_.start();
    EventLoop* loop=nullptr;

    // 使用条件变量，避免startLoop运行结束时，threadFunc还没有运行，没有正确初始化loop对象
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while(loop_==nullptr){
            cond_.wait(lock);
        }
        loop=loop_;
    }
    return loop;
}

// 该函数在单独的新的线程里面运行
void EventLoopThread::threadFunc(){
    EventLoop loop;

    // callback_--->ThreadInitCallback即loop初始化
    if(callback_){
        callback_(&loop);
    }

    
    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_=&loop;
        cond_.notify_one();
    }

    // 开启loop的事件循环  => Poller.poll
    loop.loop();

    // loop循环结束之后（循环结束）
    std::unique_lock<std::mutex> lock(mutex_);
    loop_=nullptr;
}
