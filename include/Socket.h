#pragma once
#include"noncopyable.h"

class InetAddress;

// 封装Socket fd
class Socket:noncopyable{
public:
    explicit Socket(int sockfd):sockfd_(sockfd){}
    ~Socket();
    int fd()const{return sockfd_;}
    // 
    void bindAddress(const InetAddress& localaddr);
    void listen();
    int accept(InetAddress* peeraddr);
    
    void shutdownWrite();
    // 禁用Nagle算法（降低延迟）
    void setTcpNoDelay(bool on);
    // 允许重用本地地址
    void setReuseAddr(bool on);
    // 允许重用本地端口号
    void setReusePort(bool on);
    // 启动TCP 心跳包（检查死连接）
    void setKeepAlive(bool on);
private:
    const int sockfd_;
};