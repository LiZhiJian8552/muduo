#include"../include/Poller.h"
#include "../include/Poller.h"
#include "../include/Channel.h"
#include "Poller.h"

Poller::Poller(EventLoop *loop):ownerLoop_(loop){

}


bool Poller::hasChannel(Channel *channel) const{
    // 通过fd在map中查找是否对应的Channel
    auto it=channels_.find(channel->fd());
    return it!=channels_.end() && it->second==channel;
}
