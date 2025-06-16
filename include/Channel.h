#pragma

#include "noncopyable.h"
#include "Timestamp.h"

#include<functional>
#include<memory>

class EventLoop;
// class Timestamp;
/**
 * Channel 封装了 sockfd和其感兴趣的event 如EPOLLIN和EPOLLOUT
 * 
 */
class Channel:noncopyable{
public:
        using EventCallback=std::function<void()> ;
        using ReadEventCallback=std::function<void(Timestamp)>;

        Channel(EventLoop* loop,int fd);
        ~Channel();
        
        // 调用相应的方法处理事件
        void handleEvent(Timestamp receiveTime);

        // 设置回调函数对象
        void setReadCallback(ReadEventCallback cb){ readCallback_ =std::move(cb);}
        void setWriteCallback(EventCallback cb){writeCallback_=std::move(cb);}
        void setCloseCallback(EventCallback cb){closeCallback_=std::move(cb);}
        void setErrorCallback(EventCallback cb){errorCallback_=std::move(cb);}
        
        void tie(const std::shared_ptr<void>&);

        int fd() const{ return fd_;}
        int events()const{return events_;}
        void set_revents(int revt){revents_=revt;}

        void enableReading(){events_|=KReadEvent;update();}
        void disableReading(){events_&=~KReadEvent;update();}
        void enableWriting(){events_|=KWriteEvent;update();}
        void disableWriting(){events_&=~KWriteEvent;update();}
        void disableAll(){events_=KNoneEvent;update();}

        // 返回当前fd的事件状态
        bool isNoneEvent()const {return events_==KNoneEvent;}
        bool isWriting()const{return events_&KWriteEvent;}
        bool isReading()const{return events_&KReadEvent;}

        int index(){return index_;}
        void set_index(int idx){index_=idx;}

        EventLoop* ownerLoop(){return loop_;};
        void remove();
private:
    void update();
    void handleEventWithGuard(Timestamp receiveTime);

    // 表示fd感兴趣的事件信息的描述
    static const int KNoneEvent;
    static const int KReadEvent;
    static const int KWriteEvent;

    // 事件循环
    EventLoop* loop_;
    // fd,Poller监听的对象
    const int fd_;

    // fd感兴趣的事件
    int events_;
    // poller返回的具体发生的事件
    int revents_;
    // 表示当前fd_的状态
    int index_;

    std::weak_ptr<void> tie_;
    bool tied_;

    // Channel中 可以得到fd_具体发生的事件，所以它负责调用具体事件的回调
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};