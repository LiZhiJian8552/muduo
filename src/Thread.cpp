#include"Thread.h"
#include"CurrentThread.h"

#include<semaphore.h>

// 静态变量类外初始化，原子int初始化方式
std::atomic_int Thread::numCreated_(0);

Thread::Thread(ThreadFunc func, const std::string &name)
    :started_(false),
    joined_(false),
    tid_(0),
    func_(std::move(func)),
    name_(name)
{
    setDefaultName();
}

Thread::~Thread(){
    if(started_&&!joined_){
        thread_->detach();
    }
}

void Thread::start(){
    started_=true;

    // 定义信号量
    sem_t sem;
    // 信号量初始化,初始没有资源 false->0
    sem_init(&sem,false,0);

    // 创建一个子线程，执行该线程函数
    thread_=std::shared_ptr<std::thread>(new std::thread([&](){
        // 获取当前线程tid
        tid_=CurrentThread::tid();

        sem_post(&sem);
        func_();
    }));

    // 因为子线程和当前线程执行顺序不一定，所以使用信号量确保运行到此是tid_一定是有值的
    sem_wait(&sem);
}

void Thread::join(){
    joined_=true;
    thread_->join();
}

void Thread::setDefaultName(){
    int num=++numCreated_;
    if(name_.empty()){
        char buf[32]={0};
        snprintf(buf,sizeof(buf),"Thread%d",num);
        name_=buf;
    }
}
