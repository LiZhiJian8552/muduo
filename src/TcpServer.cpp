#include"../include/TcpServer.h"
#include"../include/Logger.h"

#include<functional>


// 用户传入的Loop不能为空
EventLoop* CheckLoopNotNull(EventLoop* loop){
    if(loop==nullptr){
        LOG_FATAL("%s:%s:%d mainLoop is null!\n",__FILE__,__FUNCTION__,__LINE__);
    }
    return loop;
}

TcpServer::TcpServer(EventLoop* loop,
        const InetAddress& listenAddr,
        const std::string& nameArg,
        Option option)
        :loop_(CheckLoopNotNull(loop)),
        ipPort_(listenAddr.toIpPort()),
        name_(nameArg),
        acceptor_(new Acceptor(loop,listenAddr,option==kReusePort)),
        threadPool_(new EventLoopThreadPool(loop,name_)),
        connectionCallback_(),
        messageCallback_(),
        nextConnId_(1)    //???
{
    // 当有新用户连接时，会执行TcpServer::newConnection（根据轮询算法选择一个subloop,唤醒subloop,把当前connfd封装成channel分发给subloop）
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection,this,std::placeholders::_1,std::placeholders::_2));

}
TcpServer::~TcpServer(){}

void TcpServer::setThreadNum(int numThreads){
    threadPool_->setThreadNum(numThreads);
}

void TcpServer::start() {
    // 防止一个TcpServer对象被start多次(第二次调用start则不为0，不会进入)
    if(started_++==0){
        threadPool_->start(threadInitCallback_);
        // 在baseLoop中启动监听
        //acceptor_.get()获取acceptor裸指针
        loop_->runInLoop(std::bind(&Acceptor::listen,acceptor_.get()));
    }
}

void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr){
}

void TcpServer::removeConnection(const TcpConnectionPtr &conn){
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn){
}
