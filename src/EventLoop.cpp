#include"../include/EventLoop.h"
#include"../include/Logger.h"
#include"../include/Poller.h"
#include"../include/Channel.h"

#include<sys/eventfd.h>
#include<unistd.h>
#include<fcntl.h>
#include<errno.h>
#include "EventLoop.h"

// 当t_loopInThisThread为空时，创建EventLoop对象，如果不为空则不会创建，用于防止一个线程创建多个EventLoop
__thread EventLoop* t_loopInThisThread =nullptr;

// 定义默认的Poller IO复用接口的超时时间
const int kPollTimeMs=10000;

// 创建wakeupfd，用于notify唤醒subReactor处理新来的Channel
int createEventfd(){
    // 用于唤醒loop
    int evtfd=::eventfd(0,EFD_NONBLOCK|EFD_CLOEXEC);
    if(evtfd<0){
        LOG_FATAL("eventfd error:%d\n",errno);
    }
    return evtfd;
}




EventLoop::EventLoop()
    :looping_(false),
    quit_(false),
    callingPendingFunctors_(false),
    threadId_(CurrentThread::tid()),
    poller_(Poller::newDefaultPoller(this)),
    wakeupFd_(createEventfd()),
    wakeupChannel_(new Channel(this,wakeupFd_)), //TODO 不理解
    currentActivateChannel_(nullptr)
{
    LOG_DEBUG("EventLoop created %p in thread %d \n",this,threadId_);
    if(t_loopInThisThread){
        LOG_FATAL("Another EventLoop %p exists in this thread %d\n",t_loopInThisThread,threadId_);
    }else{
        t_loopInThisThread=this;
    }

    // 设置wakeupfd的事件类型，以及发生事件后的回调操作
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::HandleRead,this));
    // 每一个eventloop都将监听wakeupchannel的EPOLLIN读事件
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop(){
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread=nullptr; 
}


void EventLoop::HandleRead(){
    uint64_t one=1;
    ssize_t n=read(wakeupFd_,&one,sizeof(one));

    if(n!=sizeof(one)){
        LOG_ERROR("EventLoop::handleRead() reads %d bytes instead of 8",n);
    }
}