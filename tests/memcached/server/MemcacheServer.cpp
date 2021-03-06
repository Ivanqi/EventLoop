#include "MemcacheServer.h"
#include "Atomic.h"
#include "EventLoop.h"

AtomicInt64 g_cas;

MemcacheServer::Options::Options()
{
    // 把Options重置
    memZero(this, sizeof(*this));
}

struct MemcacheServer::Stats
{

};

MemcacheServer::MemcacheServer(EventLoop *loop, const Options& options)
    :loop_(loop), options_(options), startTime_(::time(NULL) - 1),
    server_(loop, InetAddress(options.tcpport), "memcaced"), stats_(new Stats)
{
    server_.setConnectionCallback(std::bind(&MemcacheServer::onConnection, this, _1));
}

MemcacheServer::~MemcacheServer() = default;

void MemcacheServer::start()
{
    server_.start();
}

void MemcacheServer::stop()
{
    loop_->runAfter(3.0, std::bind(&EventLoop::quit, loop_));
}

bool MemcacheServer::storeItem(const ItemPtr& item, const Item::UpdatePolicy policy, bool *exists)
{
    // 判断是否是完整的命令
    assert(item->neededBytes() == 0);

    MutexLock& mutex = shards_[item->hash() % kShards].mutex;
    ItemMap& items = shards_[item->hash() % kShards].items;

    MutexLockGuard lock(mutex);
    ItemMap::const_iterator it = items.find(item);
    
    *exists = it != items.end();

    // set 命令
    if (policy == Item::kSet) {
        item->setCas(g_cas.incrementAndGet());
        // 存在，就是删除，再重新插入
        if (*exists) {
            items.erase(it);
        }
        items.insert(item);
    } else {
        if (policy == Item::kAdd) { // add命令
            if (*exists) {
                return false;
            } else {
                item->setCas(g_cas.incrementAndGet());
                items.insert(item);
            }
        } else if (policy == Item::kReplace) {  // replace命令
            if (*exists) {
                item->setCas(g_cas.incrementAndGet());
                items.erase(it);
                items.insert(item);
            } else {
                return false;
            }
        } else if (policy == Item::kAppend || policy == Item::kPrepend) {   // append和prepend命令
            if (*exists) {
                const ConstItemPtr& oldItem = *it;
                int newLen = static_cast<int>(item->valueLength() + oldItem->valueLength() - 2);
                ItemPtr newItem(Item::makeItem(item->key(), oldItem->flags(), oldItem->rel_exptime(), newLen, g_cas.incrementAndGet()));

                if (policy == Item::kAppend) {
                    newItem->append(oldItem->value(), oldItem->valueLength() - 2);
                    newItem->append(item->value(), item->valueLength());
                } else {
                    newItem->append(item->value(), item->valueLength() - 2);
                    newItem->append(oldItem->value(), oldItem->valueLength());
                }

                assert(newItem->neededBytes() == 0);
                assert(newItem->endsWithCRLF());

                items.erase(it);
                items.insert(newItem);

            } else {
                return false;
            }
        } else if (policy == Item::kCas) {  // cas命令，检查然后设置
            if (*exists && (*it)->cas() == item->cas()) {
                item->setCas(g_cas.incrementAndGet());
                items.erase(it);
                items.insert(item);
            } else {
                return false;
            }
        } else {
            assert(false);
        }
    }

    return true;
}

// 寻找item
ConstItemPtr MemcacheServer::getItem(const ConstItemPtr& key) const
{
    MutexLock& mutex = shards_[key->hash() % kShards].mutex;
    const ItemMap& items = shards_[key->hash() % kShards].items;

    MutexLockGuard lock(mutex);
    ItemMap::const_iterator it = items.find(key);
    return it != items.end() ? *it : ConstItemPtr();
}

bool MemcacheServer::deleteItem(const ConstItemPtr& key)
{
    // 用 key->hash() % kShards 得到 shards_数组的下标
    MutexLock& mutex = shards_[key->hash() % kShards].mutex;
    ItemMap& items = shards_[key->hash() % kShards].items;
    MutexLockGuard lock(mutex);
    return items.erase(key) == 1;
}

// 连接毁掉
void MemcacheServer::onConnection(const TcpConnectionPtr& conn)
{
    if (conn->connected()) {    // 判断是否是连接状态
        SessionPtr session(new Session(this, conn));    // 增加用户
        MutexLockGuard lock(mutex_);
        assert(sessions_.find(conn->name()) == sessions_.end());
        sessions_[conn->name()] = session;
    } else {
        // 如果不是连接状态，就从session列表中删除
        MutexLockGuard lock(mutex_);
        assert(sessions_.find(conn->name()) != sessions_.end());
        sessions_.erase(conn->name());
    }
}