#ifndef EVENT_TEST_IDLECONNECTION_ECHO_H
#define EVENT_TEST_IDLECONNECTION_ECHO_H

#include "TcpServer.h"
#include <unordered_set>
#include <boost/circular_buffer.hpp>

class EchoServer
{
    private:
        typedef std::weak_ptr<TcpConnection> WeakTcpConnectionPtr;

    public:
        struct Entry
        {
            explicit Entry(const WeakTcpConnectionPtr& weakConn)
                :weakConn_(weakConn)
            {

            }

            ~Entry()
            {
                TcpConnectionPtr conn = weakConn_.lock();
                if (conn) {
                    conn->shutdown();
                }
            }

            WeakTcpConnectionPtr weakConn_;
        };

    private:
        typedef std::shared_ptr<Entry> EntryPtr;
        typedef std::weak_ptr<Entry> WeakEntryPtr;
        typedef std::unordered_set<EntryPtr> Bucket;
        typedef boost::circular_buffer<Bucket> WeakConnectionList;

        TcpServer server_;
        WeakConnectionList connectionBuckets_;

    public:
        EchoServer(EventLoop *loop, const InetAddress& listenAddr, int idleSeconds);

        void start();

    private:
        void onConnection(const TcpConnectionPtr& conn);

        void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time);

        void onTimer();

        void dumpConnectionBuckets() const;
};

#endif