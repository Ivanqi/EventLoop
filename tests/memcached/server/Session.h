#ifndef EVENT_TEST_MEMCACHED_SERVER_SESSION_H
#define EVENT_TEST_MEMCACHED_SERVER_SESSION_H

#include "Item.h"
#include "TcpConnection.h"
#include <boost/tokenizer.hpp>
#include <stdio.h>
using std::string;

class MemcacheServer;
class Session: public std::enable_shared_from_this<Session>
{
    private:
        enum State
        {
            kNewCommand,    // 新命令
            kReceiveValue,  // 接收值
            kDiscardValue   // 丢弃值
        };

        enum Protocol
        {
            kAscii,
            kBinary,
            kAuto
        };

        // 空间分隔符
        struct SpaceSeparator
        {
            void reset() {}
            template <typename InputIterator, typename Token>
            bool operator() (InputIterator& next, InputIterator end, Token& toke);
        };

        typedef boost::tokenizer<SpaceSeparator, const char*, StringPiece> Tokenizer;

        struct Reader;

        MemcacheServer *owner_; // MemcahceServer 实例
        TcpConnectionPtr conn_; // tcpconnect实例
        State state_;   //状态机
        Protocol protocol_; // 协议

        // current request
        string command_;    // 存储得到的命令
        bool noreply_;
        Item::UpdatePolicy policy_; // 命令策略
        ItemPtr currItem_;
        size_t bytesToDiscard_;

        // cached
        ItemPtr needle_;
        Buffer outputBuf_;

        // per session stats
        size_t bytesRead_;
        size_t requestsProcessed_;  // 请求数

        static string kLongestKey;
    
    private:
        void onMessage(const TcpConnectionPtr& conn, Buffer *buf, Timestamp);
        void onWriteComplete(const TcpConnectionPtr& conn);
        void receiveValue(Buffer* buf);
        void discardValue(Buffer* buf);

        // 如果完成请求，则返回true
        bool processRequest(StringPiece request);
        void resetRequest();
        void reply(StringPiece msg);

        bool doUpdate(Tokenizer::iterator& beg, Tokenizer::iterator end);
        void doDelete(Tokenizer::iterator& beg, Tokenizer::iterator end);

    public:
        Session(MemcacheServer *owner, const TcpConnectionPtr& conn)
            :owner_(owner), conn_(conn), state_(kNewCommand), protocol_(kAscii),
            noreply_(false), policy_(Item::kInvalid), bytesToDiscard_(0), needle_(Item::makeItem(kLongestKey, 0, 0, 2, 0)),
            bytesRead_(0), requestsProcessed_(0)
        {
            using std::placeholders::_1;
            using std::placeholders::_2;
            using std::placeholders::_3;

            conn_->setMessageCallback(std::bind(&Session::onMessage, this, _1, _2, _3));
        }

        ~Session()
        {
            printf("~Session requests processed: %d, input buffer size: %d, output buffer size: %d\n", 
            (int) requestsProcessed_, (int)conn_->inputBuffer()->internalCapacity(), (int)conn_->outputBuffer()->internalCapacity());
        }
};

typedef std::shared_ptr<Session> SessionPtr;

#endif