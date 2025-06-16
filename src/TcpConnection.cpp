#include"TcpConnection.h"
#include "Logger.h"
#include"Socket.h"
#include"Channel.h"
#include"EventLoop.h"

#include<functional>
#include<errno.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<strings.h>


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

// 当需要发送数据无法一次性通过write系统调用发送完成时，会将剩余的数据写入outputBuffer，然后再Poller中注册EPOLLOUT，触发写事件
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
//Poller-->EPOLLHUP--->channel::closeCallback --->TcpConntion::handleClose
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


// 创建连接时调用
void TcpConnection::connectEstablished(){
    setState(kConnected);
    channel_->tie(shared_from_this());
    channel_->enableReading();  //向Poller注册channel的epollin事件

    // 新连接建立，调用新连接建立时的回调
    connectionCallback_(shared_from_this());
}
// 销毁连接时调用
void TcpConnection::connectDestroyed(){
    if(state_==kConnected){
        setState(kDisconnected);
        channel_->disableAll(); //将Channel从Poller中删除，不在监听该Channel
        connectionCallback_(shared_from_this());
    }
    channel_->remove();
}



void TcpConnection::send(const std::string& buf){
    //Connection依然连接，才发送数据
    if(state_==kConnected){  // 修改为 kConnected
        // 判断eventloop对象是否在自己的线程里面
        if(loop_->isInLoopThread()){
            sendInLoop(buf.c_str(),buf.size());
        }else{
            loop_->runInLoop(std::bind(
                &TcpConnection::sendInLoop,
                this,
                buf.c_str(),
                buf.size()
            ));
        }
    }
}

void TcpConnection::shutdown(){
    if(state_==kConnected){
        setState(kConnecting);
        loop_->runInLoop(
            std::bind(&TcpConnection::shutdownInLoop,this)
        );
    }
}


/**
 * 发送数据 应用写的快  而内核发送数据慢，需要把待发送数据写入缓冲区，而且设置了水位回调，防止发送太快
 */
void TcpConnection::sendInLoop(const void* data,size_t len){
    // 已发送的数据大小
    ssize_t nwrote=0;
    // 未发送完的数据
    ssize_t remaining=len;
    // 记录是否发生错误
    bool faultError=false;

    // 连接断了则无法发送数据
    if(state_==kDisconnected){
        LOG_ERROR("disConnected,give up writing!");
        return;
    }
    // 表示channel_第一次开始写数据，并且缓冲区中没有数据（缓冲区没有数据才可以直接发送参数传入的data）
    if(!channel_->isWriting()&&outputBuffer_.readableBytes()==0){
        nwrote=::write(channel_->fd(),data,len);
        if(nwrote>=0){   //发送成功
            // 计算剩余要发送的数据大小
            remaining=len-nwrote;
            if(remaining==0&&writeCompleteCallback_){   //发送完成并且写回调不为空
                loop_->runInLoop(
                    std::bind(writeCompleteCallback_,shared_from_this())
                );
            }

        }else{  //发送失败
            nwrote=0;
            //EWOULDBLOCK由于非阻塞没有数据
            if(errno!=EWOULDBLOCK){
                LOG_ERROR("TcpConnection::sendInLoop");
                // 收到连接重置的请求
                if(errno==EPIPE||errno==ECONNRESET){
                    faultError=true;
                }
            }
        }
    }

    // 说明当前这次write并未将全部数据发送出去，将剩余的数据保存到缓冲区中
    // 并且给channel注册EPOLLOUT事件，poller发送tcp的发送缓冲区有空间，会通知相应的sock-channel,调用writeCallback_回调方法
    // 调用handleWrite方法，把发送缓冲区中的数据全部发送完成
    if(!faultError&&remaining>0){
        // 目前发送缓冲区剩余待发送数据的长度
        ssize_t oldLen=outputBuffer_.readableBytes();
        // 如果待发送缓冲区中的数据+还未发送的数据之和大于水位线，则
        if(oldLen+remaining>=highWaterMark_&&oldLen<highWaterMark_&&highWaterMarkCallback_){
            loop_->queueInLoop(std::bind(
                highWaterMarkCallback_,
                shared_from_this(),
                oldLen+remaining
            ));
        }
        // 将剩余未发送的remaining大小的数据写入缓冲区
        outputBuffer_.append((char*)data+remaining,remaining);
        if(!channel_->isWriting()){
            channel_->enableWriting();
        }
    }
}

void TcpConnection::shutdownInLoop(){
    if(!channel_->isWriting()){ //说明outputBuffer中的数据已经全部发送完成
        socket_->shutdownWrite();       //关闭写端，然后触发EPOLLHUP,然后channel检测到该事件会调用closeCallback_即TcpConnection传入的handleClose
    }
}