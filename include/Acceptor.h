#pragma once

#include"noncopyable.h"
#include"Socket.h"
#include"Channel.h"

#include<functional>

class EventLoop;
class InetAddress;

class Acceptor:noncopyable{
public:
    using NewConnectCallback=std::function<void(int sockfd,const InetAddress&)>;

    Acceptor(EventLoop* loop,const InetAddress& listenAddr,bool reuseport);
    ~Acceptor();

    void setNewConnectionCallback(const NewConnectCallback& cb){
        newConnectCallback_=cb;
    }

    bool listenning()const{return listenning_;}
    void listen();

private:
    
    void handleRead();
    EventLoop* loop_;       //acceptor用的就是用户定义的那个baseLoop，也就是MainLoop
    Socket acceptSocket_;
    Channel acceptChannel_; //将Socket封装成Channel添加到Poller中
    NewConnectCallback newConnectCallback_; //产生新连接时对应触发的回调函数
    bool listenning_;
};