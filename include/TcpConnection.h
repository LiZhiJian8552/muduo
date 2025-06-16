#pragma once

#include"noncopyable.h"
#include"InetAddress.h"
#include"Callbacks.h"
#include"Buffer.h"
#include"Timestamp.h"

#include<memory>
#include<atomic>
#include<string>


class Channel;
class EventLoop;
class Socket;

/** 
 * TcpServer=> Acceptor =>有一个新用户连接，通过accept函数那道connfd
 * =>TcpConnection 设置回调 => Channel =>Poller => Channel 的回调操作
*/
class TcpConnection:noncopyable,public std::enable_shared_from_this<TcpConnection>{
public:
    // 表示连接状态的枚举类型
    enum StateE{kDisconnected,kConnecting,kConnected,kDisconnecting};

    TcpConnection(EventLoop* loop,
                const std::string& name,
                int sockfd,
                const InetAddress& localAddr,
                const InetAddress& peerAddr);
    ~TcpConnection();

    EventLoop* getLoop()const{return loop_;}
    const std::string& name()const{return name_;}
    const InetAddress& localAddress()const{return localAddr_;}
    const InetAddress& peerAddress()const{return peerAddr_;}

    bool connected()const{return state_==kConnected;}
    bool disconnected()const{return state_==kDisconnected;}
    

    void setConnectionCallback(const ConnectionCallback& cb){connectionCallback_=cb;}
    void setMessageCallback(const MessageCallback& cb){messageCallback_=cb;}
    void setWriteCompleteCallback(const WriteCompleteCallback& cb){writeCompleteCallback_=cb;}
    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb){highWaterMarkCallback_=cb;}
    void setCloseCallback(const CloseCallback& cb){closeCallback_=cb;}
    
    // 连接建立
    void connectEstablished();
    // 连接销毁
    void connectDestroyed();
    
    void setState(StateE state){state_=state;}

    // 发送数据
    void send(const std::string& buf);
    // 关闭当前连接
    void shutdown();

private:
    void handleRead(Timestamp receiveTime);

    
    void handleWrite();
    void handleClose();
    void handleError();

    void sendInLoop(const void* message,size_t len);
    void shutdownInLoop();

   

    EventLoop* loop_;       //这里绝对不是baseloop,因为tcpConnection都是在subloop里面管理的（那如果没有设置threadNum?）

    const std::string name_;
    std::atomic_int state_;
    bool reading_;

    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;

    // 本地网络结构
    const InetAddress localAddr_;
    // 对端网络结构
    const InetAddress peerAddr_;

    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    HighWaterMarkCallback highWaterMarkCallback_;
    CloseCallback closeCallback_;

    size_t highWaterMark_;

    // 接受数据的缓冲区
    Buffer inputBuffer_;
    // 发生数据的缓冲区
    Buffer outputBuffer_;
};