#pragma
#include"noncopyable.h"
#include"EventLoop.h"
#include"InetAddress.h"
#include"Acceptor.h"
#include"EventLoopThreadPool.h"
#include"Callbacks.h"
#include"TcpConnection.h"
#include"Buffer.h"


#include<functional>
#include<memory>
#include<string>
#include<atomic>
#include<unordered_map>
/**
 * 
 */

class TcpServer:noncopyable{
public:
    using ThreadInitCallback=std::function<void(EventLoop*)>;
    using ConnectionMap=std::unordered_map<std::string,TcpConnectionPtr>;

    // 表示端口是否可重用
    enum Option{
        kNoReusePort,
        kReusePort,
    };

    TcpServer(EventLoop* loop,const InetAddress& listenAddr,const std::string& nameArg,Option option=kNoReusePort);
    ~TcpServer();

    const std::string& ipPort()const{return ipPort_;}
    const std::string&  name()const{return name_;}
    EventLoop* getLoop()const{return loop_;}

    void setThreadInitCallback(const ThreadInitCallback& cb){threadInitCallback_=cb;}
    void setConnectionCallback(const ConnectionCallback& cb){connectionCallback_=cb;}
    void setMessageCallback(const MessageCallback& cb){messageCallback_=cb;}
    void setWriteCompleteCallback(const WriteCompleteCallback& cb){writeCompleteCallback_=cb;}

    // 设置底层subloop的个数
    void setThreadNum(int numThreads);

    // 开启服务器监听，acceptor
    void start();

private:
    EventLoop* loop_;           //baseLoop，用户定义的loop

    const std::string ipPort_;
    const std::string name_;
    
    std::unique_ptr<Acceptor> acceptor_;            //运行在mainLoop，主要是监听新连接事件
    
    std::shared_ptr<EventLoopThreadPool> threadPool_;

    ConnectionCallback connectionCallback_;         //有新连接时的回调
    MessageCallback messageCallback_;               //有读写消息时的回调
    WriteCompleteCallback writeCompleteCallback_;   //消息发送完成以后的回调
    
    ThreadInitCallback threadInitCallback_;         //loop线程初始化的回调
    
    std::atomic_int started_;

    // 是一个用于生成唯一连接标识符的原子计数器
    int nextConnId_;

    ConnectionMap connections_;                     //保存所有的连接


    void newConnection(int sockfd,const InetAddress& peerAddr);
    void removeConnection(const TcpConnectionPtr& conn);
    void removeConnectionInLoop(const TcpConnectionPtr& conn);
};