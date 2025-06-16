#include"Channel.h"
#include"EventLoop.h"
#include "Logger.h"

#include<sys/epoll.h>



const int Channel::KNoneEvent=0;
const int Channel::KReadEvent=EPOLLIN|EPOLLPRI;
const int Channel::KWriteEvent=EPOLLOUT;

Channel::Channel(EventLoop *loop,int fd)
                :loop_(loop) //表明了该channel属于哪个loop
                ,fd_(fd)
                ,events_(0)
                ,revents_(0)
                ,index_(-1)
                ,tied_(false){

}

Channel::~Channel(){

}

// 一个TcpConnection新连接创建的时候 TcpConnection =>Channel，为了防止TcpConnection被销毁之后，仍然被使用
void Channel::tie(const std::shared_ptr<void> &obj){
    tie_=obj;
    tied_=true;
}

void Channel::remove(){
    // 调用EevntLoop删除当前的Channel
    loop_->removeChannel(this);
}

// 更新Poller中的channel
void Channel::update(){
    //通过channel所属的EventLoop，调用Poller的相应方法，注册fd的events事件
    loop_->updateChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime){
    if(tied_){ 
        std::shared_ptr<void> guard=tie_.lock();
        if(guard){ 
            handleEventWithGuard(receiveTime);
        }
    }else{
        handleEventWithGuard(receiveTime);
    }
}

// 根据poller监听到的channel发生的具体事件，调用相应的回调函数
void Channel::handleEventWithGuard(Timestamp receiveTime){
    LOG_INFO("channel handleEvent revents:%d\n",revents_);

    // 发生异常
    if((revents_ & EPOLLHUP)&&!(revents_ & EPOLLIN)){
        if(closeCallback_){
            closeCallback_();
        }
    }
    // 发生错误
    if(revents_ & EPOLLERR){
        if(errorCallback_){
            errorCallback_();
        }
    }
    // 发生读事件
    if(revents_ & (EPOLLIN|EPOLLPRI)){
        if(readCallback_){
            readCallback_(receiveTime);
        }
    }
    // 发生写事件
    if(revents_ & EPOLLOUT){
        if(writeCallback_){
            writeCallback_();
        }
    }
}
