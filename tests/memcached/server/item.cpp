#include "Item.h"
#include "Buffer.h"

#include <boost/functional/hash/hash.hpp>

#include <string.h>
#include <stdio.h>

Item::Item(StringPiece keyArg, uint32_t flagsArg, int exptimeArg, int valuelen, uint64_t casArg)
    : keylen_(keyArg.size()), flags_(flagsArg), rel_exptime_(exptimeArg),
    valuelen_(valuelen), receivedBytes_(0), cas_(casArg), hash_(boost::hash_range(keyArg.begin(), keyArg.end())),
    data_(static_cast<char*>(::malloc(totalLen())))
{
    assert(valuelen_ >= 2);
    assert(receivedBytes_ < totalLen());
    append(keyArg.data(), keylen_);
}

// 追加data_值
void Item::append(const char *data, size_t len)
{
    assert(len <= neededBytes());
    memcpy(data_ + receivedBytes_, data, len);  // data复制到 data_中
    receivedBytes_ += static_cast<int>(len);    // 增加receivedBytes_的值
    assert(receivedBytes_ <= totalLen());
}

void Item::output(Buffer *out, bool needCas) const
{
    out->append("VALUE ");
    out->append(data_, keylen_);

    // key信息
    string buf = std::to_string(flags_) + " " + std::to_string(valuelen_ - 2);
    if (needCas) {
        buf += " " + std::to_string(cas_);
    }

    buf += "\r\n";
    out->append(buf);
    // value信息
    out->append(value(), valuelen_);
}

void Item::resetKey(StringPiece k)
{
    assert(k.size() <= 250);
    keylen_ = k.size();
    receivedBytes_ = 0;
    append(k.data(), k.size());
    // 得到k的hash串
    hash_ = boost::hash_range(k.begin(), k.end());
}