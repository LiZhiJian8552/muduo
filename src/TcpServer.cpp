#include"TcpServer.h"
#include"Logger.h"
#include"TcpConnection.h"

#include<functional>
#include<strings.h>

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
        nextConnId_(1),
        started_(0)   
{
    // 当有新用户连接时，会执行TcpServer::newConnection（根据轮询算法选择一个subloop,唤醒subloop,把当前connfd封装成channel分发给subloop）
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection,this,std::placeholders::_1,std::placeholders::_2));

}
TcpServer::~TcpServer(){
    for(auto& item:connections_){
        // 栈上数据，会自动释放
        TcpConnectionPtr conn(item.second);
        // 释放托管对象的所有权（如果有），调用后，该智能指针不再管理任何对象
        item.second.reset();
        conn->getLoop()->runInLoop(std::bind(                
                &TcpConnection::connectDestroyed,conn
            )
        );
    }

}

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

// 有一个新的客户端的连接，acceptor会执行这个回调操作
void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr){
    // 轮询选择管理该连接的EventLoop
    EventLoop* ioLoop=threadPool_->getNextLoop();
    char buf[64]={0};
    snprintf(buf,sizeof buf,"-%s#%d",ipPort_.c_str(),nextConnId_);
    ++nextConnId_;
    std::string connName=name_+buf;

    LOG_INFO("TcpServer::newConnection [%s] -new connection [%s] from %s \n",name_.c_str(),connName.c_str(),peerAddr.toIpPort().c_str());

    // 通过sockfd获取其绑定的本机的ip地址和端口信息
    sockaddr_in local;
    ::bzero(&local,sizeof(local));
    socklen_t addrlen=sizeof local;
    if(::getsockname(sockfd,(sockaddr*)&local,&addrlen)<0){
        LOG_ERROR("sockets::getLocalAddr");
    }
    InetAddress localAddr(local);

    // 根据连接成功的sockfd，创建TcpConnection连接对象
    TcpConnectionPtr conn(new TcpConnection(
                            ioLoop,
                            connName,
                            sockfd,
                            localAddr,
                            peerAddr
    ));
    connections_[connName]=conn;
    // 下面的回调都是用户设置给TcpServer=>TcpConnection=>Channel=>Poller=>notify channel调用回调
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);

    // 设置如何关闭连接的回调
    conn->setCloseCallback(
        std::bind(&TcpServer::removeConnection,this,std::placeholders::_1)
    );
    // 直接调用TcpConnection::connectEstablished
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished,conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr &conn){
    loop_->runInLoop(std::bind(
        &TcpServer::removeConnectionInLoop,
        this,
        conn
    ));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn){
    LOG_INFO("TcpServer::removeConnectionInLoop [%s] -connection %s\n",conn->name().c_str(),name_.c_str());
    // 从记录当前存活的连接表中将当前连接删除
    size_t n=connections_.erase(conn->name());

    // 获取当前连接的Loop
    EventLoop* ioLoop=conn->getLoop();
    ioLoop->queueInLoop(std::bind(
        &TcpConnection::connectDestroyed,conn
    ));
}
