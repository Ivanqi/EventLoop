#include "echo.h"
#include "EventLoop.h"

#include <assert.h>
#include <stdio.h>

EchoServer::EchoServer(EventLoop *loop, const InetAddress& listenAddr, int idleSeconds)
    :server_(loop, listenAddr, "EchoServer"), connectionBuckets_(idleSeconds)
{
    server_.setConnectionCallback(std::bind(&EchoServer::onConnection, this, _1));

    server_.setMessageCallback(std::bind(&EchoServer::onMessage, this, _1, _2, _3));

    loop->runEvery(1.0, std::bind(&EchoServer::onTimer, this));
    
    connectionBuckets_.resize(idleSeconds);

    dumpConnectionBuckets();
}

void EchoServer::start()
{
    server_.start();
}

void EchoServer::onConnection(const TcpConnectionPtr& conn) 
{
    string state = conn->connected() ? "UP" : "DOWN";
    printf("EchoServer - %s -> %s is %s\n", conn->peerAddress().toIpPort().c_str(), conn->localAddress().toIpPort().c_str(), state.c_str());

    if (conn->connected()) {
        EntryPtr entry(new Entry(conn));
        connectionBuckets_.back().insert(entry);
        dumpConnectionBuckets();
        WeakEntryPtr weakEntry(entry);

        conn->setContext(weakEntry);
    } else {
        assert(!conn->getContext().empty());
        WeakEntryPtr weakEntry(boost::any_cast<WeakEntryPtr>(conn->getContext()));
        printf("Entry use_count = %ld\n", weakEntry.use_count());
    }
}

void EchoServer::onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time) 
{
    string msg(buf->retrieveAllAsString());
    printf("%s echo %d bytes at %s\n", conn->name().c_str(), (int)msg.size(), time.toString().c_str());

    conn->send(msg);

    assert(!conn->getContext().empty());
    WeakEntryPtr weakEntry(boost::any_cast<WeakEntryPtr>(conn->getContext()));
    EntryPtr entry(weakEntry.lock());

    if (entry) {
        connectionBuckets_.back().insert(entry);
        dumpConnectionBuckets();
    }
}

void EchoServer::onTimer()
{
    connectionBuckets_.push_back(Bucket());
    dumpConnectionBuckets();
}

void EchoServer::dumpConnectionBuckets() const
{
    printf("size = %d\n", (int)connectionBuckets_.size());
    int idx = 0;
    for (WeakConnectionList::const_iterator bucketI = connectionBuckets_.begin(); bucketI != connectionBuckets_.end();
      ++bucketI, ++idx) {
        const Bucket& bucket = *bucketI;
        printf("[%d] len = %zd: \n", idx, bucket.size());

        for (const auto& it: bucket) {
            bool connectionDead = it->weakConn_.expired();
            printf("%p(%ld)%s, ", get_pointer(it), it.use_count(), connectionDead ? " DEAD" : "");
        }
        puts("");
    }
}