#include"Poller.h"
#include "Poller.h"
#include "Channel.h"


Poller::Poller(EventLoop *loop):ownerLoop_(loop){

}


bool Poller::hasChannel(Channel *channel) const{
    // 通过fd在map中查找是否对应的Channel
    auto it=channels_.find(channel->fd());
    return it!=channels_.end() && it->second==channel;
}
