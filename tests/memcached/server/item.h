#ifndef EVENT_TEST_MEMCACHED_SERVER_ITEM_H
#define EVENT_TEST_MEMCACHED_SERVER_ITEM_H

#include "Atomic.h"
#include "StringPiece.h"
#include "Types.h"

#include <memory>

class Buffer;
class Item;

typedef std::shared_ptr<Item> ItemPtr;
typedef std::shared_ptr<const Item> ConstItemPtr;

class Item
{
    private:
        int keylen_;    // 字符串的长度
        const uint32_t flags_;
        const int rel_exptime_; // 过期时间
        const int valuelen_; // item可以存储的长度
        int receivedBytes_; // 接受到的字符的数量
        uint64_t cas_; // 原子计数器，CAS 令牌
        size_t hash_;   // string 的hash值
        char* data_;    // item的内容空间，用来存储 命令的key + value

    public:
        enum UpdatePolicy
        {
            kInvalid,   // 无效
            kSet,   // 设置
            kAdd,   // 增加
            kReplace,   // 替换
            kAppend,    // 追加
            kPrepend,   // prepend 命令用于向已存在 key(键) 的 value(数据值) 前面追加数据
            kCas    // CAS（Check-And-Set 或 Compare-And-Swap） 命令用于执行一个"检查并设置"的操作
        };

        static ItemPtr makeItem(StringPiece keyArg, uint32_t flagsArg, int exptimeArg, int valuelen, uint64_t casArg)
        {
            return std::make_shared<Item>(keyArg, flagsArg, exptimeArg, valuelen, casArg);
        }

        Item(StringPiece keyArg, uint32_t flagsArg, int exptimeArg, int valuelen, uint64_t casArg);

        ~Item()
        {
            ::free(data_);
        }

        StringPiece key() const
        {
            return StringPiece(data_, keylen_);
        }

        uint32_t flags() const
        {
            return flags_;
        }

        int rel_exptime() const
        {
            return rel_exptime_;
        }

        const char* value() const
        {
            return data_ + keylen_;
        }

        size_t valueLength() const
        {
            return valuelen_;
        }

        uint64_t cas() const
        {
            return cas_;
        }

        size_t hash() const
        {
            return hash_;
        }

        void setCas(uint64_t casArg)
        {
            cas_ = casArg;
        }

        size_t neededBytes() const
        {
            return totalLen() - receivedBytes_;
        }

        void append(const char* data, size_t len);

        bool endsWithCRLF() const
        {
            return receivedBytes_ == totalLen() && data_[totalLen() - 2] == '\r' && data_[totalLen() - 1] == '\n';
        }

        void output(Buffer *out, bool needCas = false) const;

        void resetKey(StringPiece k);
    
    private:
        int totalLen() const
        {
            return keylen_ + valuelen_;
        }
};

#endif