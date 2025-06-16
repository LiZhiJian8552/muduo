#include"Acceptor.h"
#include"Logger.h"
#include"InetAddress.h"

#include<sys/types.h>
#include<sys/socket.h>
#include <netinet/in.h>
#include<errno.h>
#include<unistd.h>


// 创建一个非阻塞的IO Socketfd
static int createNonblocking(){
    int sockfd=::socket(AF_INET,SOCK_STREAM|SOCK_NONBLOCK|SOCK_CLOEXEC,0);
    if(sockfd<0){
        LOG_FATAL("%s:%s:%d listen socket create err:%d \n",__FILE__,__FUNCTION__,__LINE__,errno);
    }
    return sockfd; 
}

Acceptor::Acceptor(EventLoop* loop,const InetAddress& listenAddr,bool reuseport)
        :loop_(loop),
        acceptSocket_(createNonblocking()), //创建socket
        acceptChannel_(loop,acceptSocket_.fd()),
        listenning_(false)
{
    // 绑定socket
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(true);
    // 将上面创建的socket绑定要listenAddr这个Channel中
    acceptSocket_.bindAddress(listenAddr);

    // TCPServer::start() acceptor.listen 当有新用户的连接时，要执行一个回调，将connfd封装成Channel，然后唤醒subloop
    //绑定新连接处理的回调
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead,this));
}
Acceptor::~Acceptor(){
    // 从epollfd中删除
    acceptChannel_.disableAll();
    // 从Poller中删除
    acceptChannel_.remove();
}



void Acceptor::listen(){
    listenning_=true;
    acceptSocket_.listen();
    // 将Channel注册到Poller中，用于监听该套接字
    acceptChannel_.enableReading();
}

//执行时机：listenfd有事件发生时（有新用户连接时）
void Acceptor::handleRead(){
    InetAddress peerAddr;
    int connfd=acceptSocket_.accept(&peerAddr);
    
    if(connfd>=0){
        if(newConnectCallback_){
            newConnectCallback_(connfd,peerAddr);
        }else{
            ::close(connfd);
        }
    }else{
        LOG_ERROR("%s:%s:%d accpet err:%d \n",__FILE__,__FUNCTION__,__LINE__,errno);
        if(errno==EMFILE){
            LOG_ERROR("%s:%s:%d sockfd reached limit \n",__FILE__,__FUNCTION__,__LINE__);
        }
    }
}