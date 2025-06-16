#include "EPollPoller.h"
#include "Logger.h"
#include "Channel.h"


#include<errno.h>
#include <unistd.h>
#include <cstring>
#include "EventLoop.h"


// 表示当前Channel的状态
const int kNew=-1;      //从未添加过poller
const int kAdded=1;     //已经添加到poller
const int kDeleted=2;   //已经从poller中删除



EPollPoller::EPollPoller(EventLoop *loop)
    :Poller(loop),
    epollfd_(::epoll_create1(EPOLL_CLOEXEC)), //epoll_create
    events_(KInitEventListSize)    //vector<epoll_event>
{
    if(epollfd_<0){ //创建失败
        LOG_FATAL("epoll_create error:%d \n",errno);
    }
}

EPollPoller::~EPollPoller(){
    ::close(epollfd_); //close---->头文件 unistd.h
}

Timestamp EPollPoller::poll(int timeoutMs, ChannelList *activateChannels){
    // 使用LOG_DEBUG更合理
    LOG_INFO("func=%s => fd total count:%lu\n",__FUNCTION__,channels_.size());
    
    int numEvents=::epoll_wait(epollfd_,&*events_.begin(),static_cast<int>(events_.size()),timeoutMs);
    
    int savedErrno=errno;
    Timestamp now(Timestamp::now());
    
    if(numEvents>0){
        LOG_INFO("%d events happened \n",numEvents);

        fillActivateChannels(numEvents,activateChannels);

        // 扩容
        if(numEvents==events_.size()){
            events_.resize(events_.size()*2);
        }
        
    }else if(numEvents==0){
        LOG_DEBUG("%s timeout! \n",__FUNCTION__); 
    }else{
        if(savedErrno!=EINTR){
            errno=savedErrno;
            LOG_ERROR("EPollPoller::poll() err!");
        }
    }
    return now;
}

void EPollPoller::fillActivateChannels(int numEvents, ChannelList *activateChannels)const{

    for(int i=0;i<numEvents;i++){
       
        Channel* channel=static_cast<Channel*>(events_[i].data.ptr);

        channel->set_revents(events_[i].events);

        activateChannels->push_back(channel);
        
    }
}



// channle update remove => EventLoop updateChannel removeChannel
/**
 *          EventLoop
 *  ChannelList     Poller
 *                  ChannelMap   <fd,Channel>
 */
void EPollPoller::updateChannel(Channel *channel){
    // 获取channel与Poller的状态
    const int index=channel->index();
    LOG_INFO("func=%s fd=%d events=%d index=%d \n",__FUNCTION__,channel->fd(),channel->events(),index);
    if(index==kNew || index==kDeleted){
        if(index==kNew){
            int fd=channel->fd();
            channels_[fd]=channel;
        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD,channel);
    }else{
        int fd=channel->fd();
        if(channel->isNoneEvent()){ //对任何事件都不感兴趣。即从poller中删除
            update(EPOLL_CTL_DEL,channel);
            channel->set_index(kDeleted);
        }else{
            update(EPOLL_CTL_MOD,channel); 
        }
    }
}


void EPollPoller::removeChannel(Channel *channel){
    int fd=channel->fd();    
    channels_.erase(fd);

    LOG_INFO("func=%s fd=%d \n",__FUNCTION__,channel->fd());

    int index=channel->index();
    if(index==kAdded){
        update(EPOLL_CTL_DEL,channel);
    }
    channel->set_index(kNew);
}

// epoll_ctl
void EPollPoller::update(int operation, Channel *channel){
    int fd=channel->fd();
    // epoll 事件
    struct epoll_event event;
    bzero(&event, sizeof event);
    
    event.events=channel->events();
    // 不能颠倒 当先设置ptr再设置fd时，fd的值会覆盖ptr的值，导致之前设置的指针被覆盖！  
    event.data.fd=fd;
    event.data.ptr=channel;
    
    
    if(::epoll_ctl(epollfd_,operation,fd,&event)<0){
        if(operation==EPOLL_CTL_DEL){
            LOG_ERROR("epoll_ctl del error:%d\n",errno);
        }else{
            LOG_FATAL("epoll_ctl add/mode error:%d",errno);
        }
    }
}
