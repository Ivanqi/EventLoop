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
        int keylen_;
        const uint32_t flags_;
        const int rel_exptime_;
        const int valuelen_;
        int receivedBytes_;
        uint64_t cas_;
        size_t hash_;
        char* data_;

    public:
        enum UpdatePolicy
        {
            kInvalid,
            kSet,   // 设置
            kAdd,   // 增加
            kReplace,   // 替换
            kAppend,    // 追加
            kPrepend,   // 预支
            kCas
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