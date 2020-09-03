#ifndef EVENT_BUFFER_H
#define EVENT_BUFFER_H

#include "StringPiece.h"
#include "Types.h"
#include "Endian.h"

#include <algorithm>
#include <vector>
#include <assert.h>
#include <string.h>

/// A buffer class modeled after org.jboss.netty.buffer.ChannelBuffer
///
/// @code
/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size
/// @endcode

class Buffer
{
    private:
        std::vector<char> buffer_;
        size_t readerIndex_;
        size_t writerIndex_;

    public:
        static const size_t kCheapPrepend = 8;
        static const size_t kInitialSize = 1024;

        explicit Buffer(size_t initialSize = kInitialSize): buffer_(kCheapPrepend + kInitialSize), 
            readerIndex_(kCheapPrepend), writerIndex_(kCheapPrepend)
        {
            assert(readableBytes() == 0);
            assert(writableBytes() == initialSize);
            assert(prependableBytes() == kCheapPrepend);
        }

        void swap(Buffer& rhs)
        {
            buffer_.swap(rhs.buffer_);
            std::swap(readerIndex_, rhs.readerIndex_);
            std::swap(writerIndex_, rhs.writerIndex_);
        }

        size_t readableBytes() const
        {
            return writerIndex_ - readerIndex_;
        }

        size_t writableBytes() const
        {
            return buffer_.size() - writerIndex_;
        }

        size_t prependableBytes() const
        {
            return readerIndex_;
        }

        const char* peek() const
        {
            return begin() + readerIndex_;
        }

    private:
        char *begin()
        {
            return &*buffer_.begin();
        }

        const char* begin() const
        {
            return &*buffer_.begin();
        }
};

#endif