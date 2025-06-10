#include"../include/TcpConnection.h"
#include "../include/Logger.h"
#include"../include/Socket.h"
#include"../include/Channel.h"
#include"../include/EventLoop.h"

#include<functional>
#include<errno.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<strings.h>
#include <netinet/tcp.h>

static EventLoop* CheckLoopNotNull(EventLoop* loop){
    if(loop==nullptr){
        LOG_FATAL("%s:%s:%d TcpConnection Loop is null!\n",__FILE__,__FUNCTION__,__LINE__);
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop *loop, 
                            const std::string &name, 
                            int sockfd, 
                            const InetAddress &localAddr, 
                            const InetAddress &peerAddr)
                            :loop_(CheckLoopNotNull(loop)),
                            name_(name),
                            state_(kConnecting),
                            reading_(true),
                            socket_(new Socket(sockfd)),
                            channel_(new Channel(loop,sockfd)),
                            localAddr_(localAddr),
                            peerAddr_(peerAddr),
                            highWaterMark_(64*1024*1024)   //TODO 作用？ 大小为64M，
{
    // 下面给channel设置相应的回调函数，poller给channel通知感兴趣的事件，相应事件发生时，channel会回调相应的操作函数
    channel_->setReadCallback(
        std::bind(&TcpConnection::handleRead,this,std::placeholders::_1)
    );
    channel_->setWriteCallback(
        std::bind(&TcpConnection::handleWrite,this)
    );
    channel_->setCloseCallback(
        std::bind(&TcpConnection::handleClose,this)
    );
    channel_->setErrorCallback(
        std::bind(&TcpConnection::handleError,this)
    );

    LOG_INFO("TcpConection::ctor[%s] at fd=%d\n",name_.c_str(),sockfd);

    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection(){
    LOG_INFO("TcpConnection::dtor[%s] at fd=%d state=%d \n",name_.c_str(),channel_->fd(),(int)state_);
}


void TcpConnection::handleRead(Timestamp receiveTime){
    int saveErrno=0;
    ssize_t n=inputBuffer_.readFd(channel_->fd(),&saveErrno);
    if(n>0){
        // 已建立连接的用户，有可读事件发生，调用用户传入的回调函数操作
        messageCallback_(shared_from_this(),&inputBuffer_,receiveTime);
    }else if(n==0){ //连接关闭
        handleClose();
    }else{  //出错
        errno=saveErrno;
        LOG_ERROR("TcpConnection::hanleRead");
        handleError();
    }
}

void TcpConnection::handleWrite(){
    // outputBuffer中可读部分才是要发送的数据
    if(channel_->isWriting()){
        int saveErrno=0;
        ssize_t n= outputBuffer_.writeFd(channel_->fd(),&saveErrno);
        if(n>0){
            // 发送了n个字节的数据，将buffer中可读标志位前移
            outputBuffer_.retrieve(n);
            // 如果此时可读字节为0，则表示发送完成
            if(outputBuffer_.readableBytes()==0){
                channel_->disableWriting();
                if(writeCompleteCallback_){ //写完成的回调函数不为空时
                    loop_->queueInLoop(
                        std::bind(writeCompleteCallback_,shared_from_this())
                    );
                }
                if(state_==kDisconnecting){
                    shutdownInLoop();
                }
            }
        }else{
            LOG_ERROR("TcpConnection::hanleWrite");
        }
    }else{  //不可写
        LOG_ERROR("TcpConnection fd=%d is down no more writing \n",channel_->fd());
    }
}

void TcpConnection::handleClose(){
    LOG_INFO("fd=%d state=%d \n",channel_->fd(),(int)state_);
    setState(kDisconnected);
    // 该channel对所有的事件都不感兴趣
    channel_->disableAll();
    
    TcpConnectionPtr connPtr(shared_from_this());
    connectionCallback_(connPtr);   //执行连接关闭的回调
    closeCallback_(connPtr);    //关闭连接的回调
}

// 处理错误
void TcpConnection::handleError(){
    int optval;
    socklen_t optlen=sizeof optval;

    int err=0;
    if(::getsockopt(channel_->fd(),SOL_SOCKET,SO_ERROR,&optval,&optlen)<0){
        // getsockopt调用失败的原因
        err=errno;
    }else{
        // 获取fd出错的原因
        err=optval;
    }
    LOG_ERROR("TcpConnection::handleError name:%s - SO_ERROR:%d",name_.c_str(),err);
}
