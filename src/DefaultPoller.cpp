#include"Poller.h"
#include"EPollPoller.h"
#include<stdlib.h>


Poller* Poller::newDefaultPoller(EventLoop* loop){
    if(::getenv("MUDUO_USE_POLL")){
        // 生成Poll实例
        return nullptr;
    }else{
        // 生成Epoll实例
        return new EPollPoller(loop);
    }
}