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

void Item::append(const char *data, size_t len)
{
    assert(len <= neededBytes());
    memcpy(data_ + receivedBytes_, data, len);
    receivedBytes_ += static_cast<int>(len);
    assert(receivedBytes_ <= totalLen());
}

void Item::output(Buffer *out, bool needCas) const
{
    out->append("VALUE ");
    out->append(data_, keylen_);

    string buf = std::to_string(flags_) + " " + std::to_string(valuelen_ - 2);
    if (needCas) {
        buf += " " + std::to_string(cas_);
    }

    buf += "\r\n";
    out->append(buf);
    out->append(value(), valuelen_);
}

void Item::resetKey(StringPiece k)
{
    assert(k.size() <= 250);
    keylen_ = k.size();
    receivedBytes_ = 0;
    append(k.data(), k.size());
    hash_ = boost::hash_range(k.begin(), k.end());
}