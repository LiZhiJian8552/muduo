#include"EventLoop.h"
#include"Logger.h"
#include"Poller.h"
#include"Channel.h"

#include<sys/eventfd.h>
#include<unistd.h>
#include<fcntl.h>
#include<errno.h>

// 当t_loopInThisThread为空时，创建EventLoop对象，如果不为空则不会创建，用于防止一个线程创建多个EventLoop
__thread EventLoop* t_loopInThisThread =nullptr;

// 定义默认的Poller IO复用接口的超时时间
const int kPollTimeMs=60000;

// 创建wakeupfd，用于notify唤醒subReactor处理新来的Channel
int createEventfd(){
    // 用于唤醒loop
    int evtfd=::eventfd(0,EFD_NONBLOCK|EFD_CLOEXEC);
    if(evtfd<0){
        LOG_FATAL("eventfd error:%d\n",errno);
    }
    return evtfd;
}



EventLoop::EventLoop()
    :looping_(false),
    quit_(false),
    callingPendingFunctors_(false),
    threadId_(CurrentThread::tid()),
    poller_(Poller::newDefaultPoller(this)),
    wakeupFd_(createEventfd()),
    wakeupChannel_(new Channel(this,wakeupFd_)) //TODO 不理解
    // currentActivateChannel_(nullptr)
{
    LOG_DEBUG("EventLoop created %p in thread %d \n",this,threadId_);
    if(t_loopInThisThread){
        LOG_FATAL("Another EventLoop %p exists in this thread %d\n",t_loopInThisThread,threadId_);
    }else{
        t_loopInThisThread=this;
    }

    // 设置wakeupfd的事件类型，以及发生事件后的回调操作
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead,this));
    // 每一个eventloop都将监听wakeupchannel的EPOLLIN读事件
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop(){
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread=nullptr; 
}


void EventLoop::handleRead(){
    uint64_t one=1;
    ssize_t n=read(wakeupFd_,&one,sizeof(one));

    if(n!=sizeof(one)){
        LOG_ERROR("EventLoop::handleRead() reads %ld bytes instead of 8",n);
    }
}

// 开启事件循环
void EventLoop::loop(){
    looping_=true;
    quit_=false;

    LOG_INFO("EventLoop %p start looping \n",this);

    while(!quit_){
        activateChannels_.clear();

        // Poller监听哪些channel发生了事件，然后上报给EventLoop,通知Channel处理相应的事件
        // 返回epoll_wait的有事件的Channel集合,监听两类fd,一类是client_fd,另一类是wakeupfd_
        pollReturnTime_=poller_->poll(kPollTimeMs,&activateChannels_);
        for(Channel* channel:activateChannels_){
            channel->handleEvent(pollReturnTime_);
        }
        // 执行当前EventLoop事件循环需要处理的回调操作 
        /**
         * IO线程 mainLoop ->accept   ->fd  -->Channel
         * mainLoop 事先注册一个回调cb (需要subloop来执行)    wakeup 唤醒subloop后执行下面的方法(之前mainloop注册的cb)
         */
        doPendingFunctors();
    }

    LOG_INFO("EventLoop %p stop looping. \n",this);
    looping_=false;
}

 // 退出事件循环, 1.loop在自己的线程中调用quit, 2.在其他线程中调用loop的quit
void EventLoop::quit(){
    quit_=true;

    if(!isInLoopThread()){  //如果是在其他线程中，调用的quit,在一个subloop(worker)中，调用了mainloop(IO)的quit
        wakeup();
    }
}

 // 在当前loop中执行cb
void EventLoop::runInLoop(Functor cb){
    if(isInLoopThread()){  //在当前loop线程中，执行cb
        cb();
    }else{  //在非当前loop线程中执行cb,就需要唤醒loop所在线程，执行cb
        queueInLoop(cb);
    }
}

// 将cb放入队列中，唤醒loop所在的线程，执行cb
void EventLoop::queueInLoop(Functor cb){
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }
    // 唤醒相应的需要执行上面回调操作的线程(是怎么判断是否是相应的线程呢)
    if(!isInLoopThread()||callingPendingFunctors_){
        wakeup();
    }
}

// 唤醒loop所在线程,向wakeupfd_写有一个数据，wakeupChannel就发生读事件，当前loop线程就会被唤醒
void EventLoop::wakeup(){
    uint64_t one =1;
    ssize_t n=write(wakeupFd_,&one,sizeof(one));
    if(n!=sizeof(one)){
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8\n",n);
    }
}

void EventLoop::doPendingFunctors(){
    // 创建一个临时的Functor集合，用于存储需要执行的回调操作,避免一直占有锁，影响其他线程向当前loop中添加回调
    std::vector<Functor> functors;
    callingPendingFunctors_=true;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_); 
    }
    for(const Functor& functor :functors){
        functor();
    }
    callingPendingFunctors_=false;
}


void EventLoop::updateChannel(Channel* channel){
    poller_->updateChannel(channel);
}
void EventLoop::removeChannel(Channel* channel){
    poller_->removeChannel(channel);
}
void EventLoop::hasChannel(Channel* channel){
    poller_->hasChannel(channel);
}
