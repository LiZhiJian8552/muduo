#pragma once

#include<netinet/in.h>
#include <arpa/inet.h>
#include<string>


// 封装IP+Port
class InetAddress{
private:
    struct sockaddr_in addr_;
public:
    explicit InetAddress(uint16_t port=0,std::string ip="127.0.0.1");
    explicit InetAddress(const struct sockaddr_in& addr):addr_(addr){}

    std::string toIp()const;
    std::string toIpPort()const;
    uint16_t toPort()const;
    const struct sockaddr_in * getsockAddr()const{return &addr_;}
    void setSockAddr(const sockaddr_in& addr){addr_=addr;}
};