#pragma once

#include "noncopyable.h"
#include<functional>
#include<thread>
#include<memory>
#include<unistd.h>
#include<string>
#include<atomic>


class Thread:noncopyable{
public:
    // Thread需要执行的函数
    using ThreadFunc=std::function<void()>;

    explicit Thread(ThreadFunc func,const std::string& name=std::string());
    ~Thread();
    // 启动
    void start();
    // 分离
    void join();
    // 判断是否开始
    bool started()const{return started_;}
    // 获取线程的id
    pid_t tid()const{return tid_;}
    // 返回线程名字
    const std::string& name()const{return name_;}
    // 返回
    static int numCreated(){return numCreated_;}
private:
    bool started_;
    bool joined_;
    
    std::shared_ptr<std::thread> thread_;
    pid_t tid_;
    ThreadFunc func_;

    std::string name_;
    static std::atomic_int numCreated_;


    void setDefaultName();
};