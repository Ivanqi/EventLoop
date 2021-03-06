#include "codec.h"
#include "MutexLock.h"
#include "EventLoopThread.h"
#include "TcpClient.h"

#include <iostream>
#include <stdio.h>
#include <unistd.h>

class ChatClient
{
    private:
        TcpClient client_;
        LengthHeaderCodec codec_;
        MutexLock mutex_;
        TcpConnectionPtr connection_;
    
    public:
        ChatClient(EventLoop *loop, const InetAddress& serverAddr)
            :client_(loop, serverAddr, "ChatClient"), codec_(std::bind(&ChatClient::onStringMessage, this, _1, _2, _3))
        {
            client_.setConnectionCallback(std::bind(&ChatClient::onConnection, this, _1));

            client_.setMessageCallback(std::bind(&LengthHeaderCodec::onMessage, &codec_, _1, _2, _3));

            client_.enableRetry();
        }

        void connect()
        {
            client_.connect();
        }

        void disconnect()
        {
            client_.disconnect();
        }

        /**
         * write()会由main线程调用，所以要加锁
         * 这个锁不是为了保护TcpConnection，而是为了保护shared_ptr
         *  */
        void write(const StringPiece& message)
        {
            MutexLockGuard lock(mutex_);
            if (connection_) {
                codec_.send(get_pointer(connection_), message);
            }
        }
    
    private:
        /**
         * onConnection()会由EventLoop线程调用，所以要加锁以保护shared_ptr
         */
        void onConnection(const TcpConnectionPtr& conn)
        {
            MutexLockGuard lock(mutex_);
            if (conn->connected()) {
                connection_ = conn;
            } else {
                connection_.reset();
            }
        }

        /**
         * 把收到的信息打印到屏幕。这个函数由EventLoop线程调用，但是不用加锁，因为printf()是线程安全
         * 注意这里不能用 std::cout <<，它不是线程安全
         */
        void onStringMessage(const TcpConnectionPtr&, const string& message, Timestamp)
        {
            printf("<<< %s\n", message.c_str());
        }
};

int main(int argc, char *argv[]) {

    printf("pid = %d\n", getpid());
    if (argc > 2) {
        EventLoopThread loopThread;
        uint16_t port = static_cast<uint16_t>(atoi(argv[2]));
        InetAddress serverAddr(argv[1], port);

        ChatClient client(loopThread.startLoop(), serverAddr);
        client.connect();

        std::string line;
        
        while (std::getline(std::cin, line)) {
            client.write(line);
        }

        client.disconnect();

        CurrentThread::sleepUsec(1000 * 1000);
    } else {
        printf("Usage: %s host_ip port\n", argv[0]);
    }
    return 0;
}