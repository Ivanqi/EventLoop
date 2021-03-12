#ifndef EVENT_TEST_MEMCACHED_SERVER_SESSION_H
#define EVENT_TEST_MEMCACHED_SERVER_SESSION_H

#include "Item.h"
#include "TcpConnection.h"
#include <boost/tokenizer.hpp>
using std::string;

class MemcacheServer;
class Session: public std::enable_shared_from_this<Session>, muduo::noncopyable
{
    private:
        enum State
        {
            kNewCommand,
            kReceiveValue,
            kDiscardValue
        };

        enum Protocol
        {
            kAscii,
            kBinary,
            kAuto
        };

        struct SpaceSeparator
        {
            void reset() {}
            template <typename InputIterator, typename Token>
            bool operator() (InputIterator& next, InputIterator end, Token& toke);
        };

        typedef boost::tokenizer<SpaceSeparator, const char*, StringPiece> Tokenizer;

        struct Reader;

        MemcacheServer *owner_;
        TcpConnectionPtr conn_;
        State state_;
        Protocol protocol_;

        // current request
        string command_;
        bool noreply_;
        Item::UpdatePolicy policy_;
        ItemPtr currItem_;
        size_t bytesToDiscard_;

        // cached
        ItemPtr needle_;
        Buffer ouputBuf_;

        // per session stats
        size_t bytesRead_;
        size_t requestProcessed_;

        static string kLongestKey;
};

typedef std::shared_ptr<Session> SessionPtr;

#endif