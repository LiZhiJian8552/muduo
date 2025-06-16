#pragma once

#include"Poller.h"
#include"Timestamp.h"

#include<vector>
#include<sys/epoll.h>


class Channel;
/**
 * epoll的使用
 * 1. epoll_create
 * 2. epoll_ctl   add/mod/del
 * 3. epoll_wait
 */

class EPollPoller:public Poller{
public:
    EPollPoller(EventLoop* loop);
    virtual ~EPollPoller()override;

    // 重写基类Poller的抽象方法
    Timestamp poll(int timeoutMs,ChannelList* activateChannels) override;
    void updateChannel(Channel* channel)override;
    void removeChannel(Channel* channel)override;



private:
    static const int KInitEventListSize=16;

    using EventList=std::vector<struct epoll_event>;

    int epollfd_;
    EventList events_;

    // 填写活跃的链接
    void fillActivateChannels(int numEvents,ChannelList* activateChannels)const;
    // 更新Channel
    void update(int operation,Channel* channel);
};