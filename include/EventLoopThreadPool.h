#pragma once
#include"noncopyable.h"

#include<functional>
#include<string>
#include<vector>
#include<memory>


class EventLoop;
class EventLoopThread;

// 管理EventLoopThread
class EventLoopThreadPool:noncopyable{
public:
    using ThreadInitCallback=std::function<void(EventLoop* loop)>;

    EventLoopThreadPool(EventLoop* baseLoop,const std::string& nameArg);
    ~EventLoopThreadPool();

    void setThreadNum(int numThreads){numThreads_=numThreads;}

    void start(const ThreadInitCallback &cb=ThreadInitCallback());

    // 如果工作在多线程中，baseLoop_默认以轮询的方式分配Channel给subloop
    EventLoop* getNextLoop();


    std::vector<EventLoop*> getAllLoops();

    bool started()const{return started_;}
    const std::string name()const{return name_;}
private:

    EventLoop* baseLoop_;           //用户使用的线程 ---->用户创建的EventLoop loop;
    std::string name_;
    bool started_;
    int numThreads_;
    int next_;                      //轮询下标
    std::vector<std::unique_ptr<EventLoopThread>> threads_;
    std::vector<EventLoop*> loops_;  //如果numThreads_>1?否则只有baseLoop_还是为空呢  -->为空
};