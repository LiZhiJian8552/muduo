#pragma once
#include "noncopyable.h"
#include"Timestamp.h"

#include<vector>
#include<unordered_map>

class Channel;
class EventLoop;

// IO复用模块
class Poller:noncopyable{
public:
    // 存储该Poller监听的Channel
    using ChannelList=std::vector<Channel*>;

    Poller(EventLoop* loop);
    virtual ~Poller()=default;

    // 给所有IO复用保留统一的接口
    virtual Timestamp poll(int timeoutMs,ChannelList* activateChannels)=0;
    virtual void updateChannel(Channel* channel)=0;
    virtual void removeChannel(Channel* channel)=0;

    // 判断Poller中是否阐述Channel
    bool hasChannel(Channel* channel)const;

    // 获取当前默认的Poller (可以是Epoll或者是Poll)
    static Poller* newDefaultPoller(EventLoop* loop);
protected:
    // int表示sockfd,Channel表示该fd的Channel类型，可以通过fd快速查找Channel,
    //该方法不在对应的cpp文件中定义，因为需要用到具体的EPoll或Poll,而该头文件是他们的抽象
    //将该方法放到一个单独的源文件中去实现，不用影响继承结构
    using ChannelMap=std::unordered_map<int,Channel*>;
    ChannelMap channels_;
private:
    // 定义该Poller所属的EventLoop
    EventLoop *ownerLoop_;
};