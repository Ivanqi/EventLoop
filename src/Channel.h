#ifndef EVENT_CHANNEL_H
#define EVENT_CHANNEL_H

#include "Timestamp.h"
#include <functional>
#include <memory>

class EventLoop;

/**
 * 可选择的I/O通道
 * 
 * 此类不拥有文件描述符
 * 文件描述符可以是套接字
 * 事件FD，定时器FD或者信号FD
 */
class Channel
{
    public:
        typedef std::function<void()> EventCallback;
        typedef std::function<void(Timestamp)> ReadEventCallback;

    private:

        static const int kNoneEvent;

        static const int kReadEvent;

        static const int kWriteEvent;

        EventLoop *loop_;

        const int fd_;

        int events_;    //  poll专属事件

        int revents_;   // 接收epoll或poll事件类型

        int index_;     // 由 Poller使用。用于给Poller判断是否需要增加，修改，删除事件

        bool logHup_;

        std::weak_ptr<void> tie_;

        bool tied_;

        bool eventHandling_;

        bool addedToLoop_;

        ReadEventCallback readCallback_;

        EventCallback writeCallback_;

        EventCallback closeCallback_;

        EventCallback errorCallback_;
    
    public:
        Channel(EventLoop *loop, int f);

        ~Channel();

        void handleEvent(Timestamp receiveTime);

        void setReadCallback(ReadEventCallback cb)
        {
            readCallback_ = std::move(cb);
        }

        void setWriteCallback(EventCallback cb)
        {
            writeCallback_ = std::move(cb);
        }

        void setCloseCallback(EventCallback cb)
        {
            closeCallback_ = std::move(cb);
        }

        void setErrorCallback(EventCallback cb)
        {
            errorCallback_ = std::move(cb);
        }

        /**
         * 将 channel 绑定到shared_ptr管理的所有者对象
         * 防止所有者对象在handleEvent中被销毁
         */
        void tie(const std::shared_ptr<void>&);

        int fd() const
        {
            return fd_;
        }

        int events() const
        {
            return events_;
        }

        void set_revents(int revt)
        {
            revents_ = revt;
        }

        bool isNoneEvent() const
        {
            return events_ == kNoneEvent;
        }

        void enableReading()
        {
            events_ |= kReadEvent; 
            update();
        }

        void disableReading() 
        { 
            events_ &= ~kReadEvent; 
            update(); 
        }

        void enableWriting() 
        { 
            events_ |= kWriteEvent; 
            update();
        }

        void disableWriting() 
        { 
            events_ &= ~kWriteEvent; 
            update(); 
        }

        void disableAll() 
        { 
            events_ = kNoneEvent; 
            update(); 
        }

        bool isWriting() const 
        { 
            return events_ & kWriteEvent; 
        }

        bool isReading() const 
        { 
            return events_ & kReadEvent; 
        }

         // for Poller
        int index() 
        { 
            return index_; 
        }

        void set_index(int idx) 
        { 
            index_ = idx;
        }

        // for debug
        string reventsToString() const;

        string eventsToString() const;

        void doNotLogHup() 
        { 
            logHup_ = false;
        }

        EventLoop* ownerLoop() 
        { 
            return loop_;
        }
        
        void remove();

    private:
        static string eventsToString(int fd, int ev);

        void update();

        void handleEventWithGuard(Timestamp receiveTime);
};

#endif