// 对编写的mymudu库进行简单的测试和使用
#include<mymuduo/TcpServer.h>
#include<mymuduo/Logger.h>
#include<string>
#include<functional>

class EchoServer{
public:
    EchoServer(EventLoop* loop,
            const InetAddress& addr,
            const std::string& name)
        :loop_(loop),
        server_(loop,addr,name)
    {
        // 注册回调函数
        server_.setConnectionCallback(std::bind(&EchoServer::onConnection,this,std::placeholders::_1));
        server_.setMessageCallback(std::bind(&EchoServer::onMessgae,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));
        // 设置合适的loop(subloop)线程数量
        server_.setThreadNum(3);
        
    }
    void start(){
        server_.start();
    }
private:
    //连接建立或断开的回调 
    void onConnection(const TcpConnectionPtr& conn){
        // 建立连接
        if(conn->connected()){
            LOG_INFO("conn UP:%s",conn->peerAddress().toIpPort().c_str());
        }else{
            LOG_INFO("conn DOWN:%s",conn->peerAddress().toIpPort().c_str());
        }
    }
    // 可读写事件回调
    void onMessgae(const TcpConnectionPtr& conn,Buffer* buf,Timestamp time){
        std::string msg=buf->retrieveAsAllString();
        conn->send(msg);
        // 关闭写段，触发EPOLLHUP-> closeCallback_
        // conn->shutdown();
    }
    EventLoop* loop_;
    TcpServer server_;
};

int main(){
    EventLoop loop;
    InetAddress addr(8000);
    EchoServer server(&loop,addr,"");    //创建Acceptor non-blocking listenfd create bind
    server.start(); //listen loopthread listenfd => acceptChannel => mainLoop =>
    loop.loop();    //开启baseLoop的loop循环
    return 0;
}