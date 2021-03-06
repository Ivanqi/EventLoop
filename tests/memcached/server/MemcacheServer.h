#ifndef EVENT_TEST_MEMCACHED_SERVER_MEMCACHESERVER_H
#define EVENT_TEST_MEMCACHED_SERVER_MEMCACHESERVER_H

#include "Item.h"
#include "Session.h"
#include "MutexLock.h"
#include "TcpServer.h"

#include <array>
#include <unordered_set>
#include <unordered_map>

typedef std::unordered_map<std::string, int64_t> WordCountMap;

class MemcacheServer
{
    
    public:
        struct Options 
        {
            Options();
            uint16_t tcpport;   // tcp 端口
            uint16_t udpport;   // udp 端口
            uint16_t gperfport;
            int threads;    // 线程
        };
    private:
        EventLoop *loop_;   // 主线程loop
        Options options_;   // 选项
        const time_t startTime_;    // 启动时间
        mutable MutexLock mutex_;   // 互斥锁
        TcpServer server_;
        std::unordered_map<string, SessionPtr> sessions_;   // 用户列表
        struct Stats;

        // 节省内存的复杂解决方案
        struct Hash
        {
            size_t operator()(const ConstItemPtr& x) const
            {
                return x->hash();
            }
        };

        struct Equal
        {
            bool operator()(const ConstItemPtr& x, const ConstItemPtr& y) const
            {
                return x->key() == y->key();
            }
        };

        typedef std::unordered_set<ConstItemPtr, Hash, Equal> ItemMap;

        struct MapWithLock
        {
            ItemMap items;
            mutable MutexLock mutex;
        };

        const static int kShards = 4096;
        /**
         * 用array存储item
         * 通过array + set形成一个hash table
         * 
         * 这里shards_设置得比较大，因为shards_不是一个动态hash_table
         * */
        std::array<MapWithLock, kShards> shards_;   
        std::unique_ptr<Stats> stats_;

    public:
        MemcacheServer(EventLoop *loop, const Options&);
        ~MemcacheServer();

        void setThreadNum(int threads)
        {
            server_.setThreadNum(threads);
        }

        void start();

        void stop();

        time_t startTime() const
        {
            return startTime_;
        }

        bool storeItem(const ItemPtr& item, Item::UpdatePolicy policy, bool *exists);

        ConstItemPtr getItem(const ConstItemPtr& key) const;

        bool deleteItem(const ConstItemPtr& key);
    
    private:
        void onConnection(const TcpConnectionPtr& conn);
};

#endif