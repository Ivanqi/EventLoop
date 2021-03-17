#include "Session.h"
#include "MemcacheServer.h"

static bool inBinaryProtocol(uint8_t firstByte)
{
    return firstByte == 0x80;
}

const int kLongestKeySize = 250;
string Session::kLongestKey(kLongestKeySize, 'x');

template <typename InputIterator, typename Token>
bool Session::SpaceSeparator::operator()(InputIterator& next, InputIterator end, Token& tok)
{
    // next 不等于end, next 不等于空格符
    while (next != end && *next == ' ') {
        ++next;
    }

    // next == end，意味着是空白字符
    if (next == end) {
        tok.clear();
        return false;
    }

    InputIterator start(next);
    const char *sp = static_cast<const char*>(memchr(start, ' ', end - start)); // 寻找空白字符
    // 如果有空白字符，那么以空白字符为分隔
    if (sp) {
        tok.set(start, static_cast<int>(sp - start));
        next = sp;
    } else {
        tok.set(start, static_cast<int>(end - next));
        next = end;
    }

    return true;
}

struct Session::Reader
{
    private:
        Tokenizer::iterator first_;
        Tokenizer::iterator last_;

    public:
        Reader(Tokenizer::iterator& beg, Tokenizer::iterator end)
            :first_(beg), last_(end)
        {

        }

        template<typename T>
        bool read(T *val)
        {
            if (first_ == last_) {
                return false;
            }

            char *end = NULL;
            // 将字符串转换为无符号长整型整数
            uint64_t x = strtoul((*first_).data(), &end, 10);
            if (end == (*first_).end()) {
                *val = static_cast<T>(x);
                ++first_;
                return true;
            }
            return true;
        }
};

// 读入信息
void Session::onMessage(const TcpConnectionPtr& conn, Buffer *buf, Timestamp)
{
    const size_t initalReadable = buf->readableBytes();

    // buf 的可读空间大于0
    while (buf->readableBytes() > 0) {
        // state_ 初始值为kNewCommand
        if (state_ == kNewCommand) {
            if (protocol_ == kAuto) {
                assert(bytesRead_ == 0);
                protocol_ = inBinaryProtocol(buf->peek()[0]) ? kBinary : kAscii;
            }

            assert(protocol_ == kAscii || protocol_ == kBinary);

            if (protocol_ == kBinary) {

            } else {    // ASCII protocol
                const char *crlf = buf->findCRLF(); // 寻找\r\n, \r\n为单条命令的结束符
                if (crlf) {
                    int len = static_cast<int>(crlf - buf->peek()); // 获取单条命令的长度
                    StringPiece request(buf->peek(), len);  // 命令
                    if (processRequest(request)) {
                        resetRequest();
                    }
                    buf->retrieveUntil(crlf + 2);
                } else {
                    if (buf->readableBytes() > 1024) {
                        conn_->shutdown();  // 关闭连接
                    }
                    break;
                }
            }
        } else if (state_ == kReceiveValue) {
            receiveValue(buf);
        } else if (state_ == kDiscardValue) {
            discardValue(buf);
        } else {
            assert(false);
        }
    }

    bytesRead_ += initalReadable - buf->readableBytes();
}

// 接收value
void Session::receiveValue(Buffer *buf)
{
    assert(currItem_.get());
    assert(state_ == kReceiveValue);

    // 设置命令的值的长度
    const size_t avail = std::min(buf->readableBytes(), currItem_->neededBytes());
    // 保证currItem_没有被其他线程修改
    assert(currItem_.unique());

    // 追加 item的value
    currItem_->append(buf->peek(), avail);
    buf->retrieve(avail);

    if (currItem_->neededBytes() == 0) {
        // 命令结束
        if (currItem_->endsWithCRLF()) {
            bool exists = false;
            // 存储完整的命令，并向客户段返回信息
            if (owner_->storeItem(currItem_, policy_, &exists)) {
                reply("STORED\r\n");
            } else {
                if (policy_ == Item::kCas) {
                    if (exists) {
                        reply("EXISTS\r\n");
                    } else {
                        reply("NOT_FOUND\r\n");
                    }
                } else {
                    reply("NOT_STORED\r\n");
                }
            }
        } else {
            reply("CLIENT_ERROR bad data chunk\r\n");
        }

        // 一条命令完成，重置请求
        resetRequest();
        state_ = kNewCommand;   // 设置成接收新的指令
    }
}

// 进行需要丢弃的数据进行处理
void Session::discardValue(Buffer *buf)
{
    assert(!currItem_);
    assert(state_ == kDiscardValue);

    if (buf->readableBytes() < bytesToDiscard_) {
        bytesToDiscard_ -= buf->readableBytes();
        buf->retrieveAll();
    } else {
        buf->retrieve(bytesToDiscard_);
        bytesToDiscard_ = 0;
        resetRequest();
        state_ = kNewCommand;
    }
}

// 请求处理
bool Session::processRequest(StringPiece request)
{
    assert(command_.empty());
    assert(!noreply_);
    assert(policy_ == Item::kInvalid);
    assert(!currItem_);
    assert(bytesToDiscard_ == 0);

    ++requestsProcessed_;

    // 在请求行末尾选中“noreply”
    if (request.size() >= 8) {
        StringPiece end(request.end() - 8, 0);
        if (end == " noreply") {
            noreply_ = true;
            request.remove_suffix(8);
        }
    }

    SpaceSeparator sep;
    Tokenizer tok(request.begin(), request.end(), sep);
    Tokenizer::iterator beg = tok.begin();

    // 空白命令，返回error
    if (beg == tok.end()) {
        reply("ERROR\r\n");
        return true;
    }

    (*beg).CopyToString(&command_); // 得到第一个字符
    ++beg;  // beg的下一个内存地址，指向下一个个字符

    if (command_ == "set" || command_ == "add" || command_ == "replace" || command_ == "append" || command_ == "prepend" || command_ == "cas") {
        // 这通常返回false
        return doUpdate(beg, tok.end());
    } else if (command_ == "get" || command_ == "gets") {
        /**
         * get 命令
         *  get key
         *  get key1 key2 key3
         * 
         * gets 命令
         *  gets key
         *  gets key1 key2 key3
         *  
         */
        bool cas = command_ == "gets";

        // FIXME：使用write complete回调发送多个块
        while (beg != tok.end()) {
            StringPiece key = *beg;
            bool good = key.size() <= kLongestKeySize;
            if (!good) {
                reply("CLIENT_ERROR bad command line format\r\n");
                return true;
            }

            needle_->resetKey(key);
            ConstItemPtr item = owner_->getItem(needle_);
            ++beg;
            
            // item输出
            if (item) {
                item->output(&outputBuf_, cas);
            }
        }

        outputBuf_.append("END\r\n");

        if (conn_->outputBuffer()->writableBytes() > 65536 + outputBuf_.readableBytes()) {
            printf("shrink output buffer from %d \n", (int)conn_->outputBuffer()->internalCapacity());
            conn_->outputBuffer()->shrink(65536 + outputBuf_.readableBytes());
        }

        conn_->send(&outputBuf_);
    } else if (command_ == "delete") {
        doDelete(beg, tok.end());
    } else if (command_ == "version") {
        reply("VERSION 0.01 memcached\r\n");
    } else if (command_ == "quit") {
        conn_->shutdown();
    } else if (command_ == "shutdown") {
        //“错误：未启用关机”
        conn_->shutdown();
        owner_->stop();
    } else {
        reply("ERROR\r\n");
        printf("Unknwon command: %s\n", command_.c_str());
    }

    return true;
}

// 所有请求完成，进行reset
void Session::resetRequest()
{
    command_.clear();
    noreply_ = false;
    policy_ = Item::kInvalid;
    currItem_.reset();
    bytesToDiscard_ = 0;
}

void Session::reply(StringPiece msg)
{
    if (!noreply_) {
        conn_->send(msg.data(), msg.size());
    }
}

/**
 * memcache命令
 *  set key flags exptime bytes [noreply]
 *  add key flags exptime bytes [noreply]
 *  replace key flags exptime bytes [noreply]
 *  append key flags exptime bytes [noreply]
 *  prepend key flags exptime bytes [noreply]
 *  cas key flags exptime bytes unique_cas_token [noreply]
 * 
 * 参数解释
 *  key：键值 key-value 结构中的 key，用于查找缓存值
 *  flags：可以包括键值对的整型参数，客户机使用它存储关于键值对的额外信息
 *  exptime：在缓存中保存键值对的时间长度（以秒为单位，0 表示永远）
 *  bytes：在缓存中存储的字节数
 *  noreply（可选）： 该参数告知服务器不需要返回数据
 *  unique_cas_token通过 gets 命令获取的一个唯一的64位值
 *  value：存储的值（始终位于第二行）（可直接理解为key-value结构中的value）
 */
bool Session::doUpdate(Session::Tokenizer::iterator& beg, Session::Tokenizer::iterator end)
{
    if (command_ == "set") {
        policy_ = Item::kSet;
    } else if (command_ == "add") {
        policy_ = Item::kAdd;
    } else if (command_ == "replace") {
        policy_ = Item::kReplace;
    } else if (command_ == "append") {
        policy_ = Item::kPrepend;
    } else if (command_ == "cas") {
        policy_ = Item::kCas;
    } else {
        assert(true);
    }

    // FIXME: check (beg != end)
    StringPiece key = (*beg);   // 等到键名
    ++beg;
    bool good = key.size() <= kLongestKeySize;

    uint32_t flags = 0;
    time_t exptime = 1;
    int bytes = -1;
    uint64_t cas = 0;

    Reader r(beg, end);

    // 对beg的内容进行解析.
    good = good && r.read(&flags) && r.read(&exptime) && r.read(&bytes);

    int rel_exptime = static_cast<int>(exptime);

    // 如果 exptime 大于30天，重新设置rel_exptime的值
    if (exptime > 60 * 60 * 24 * 30) {
        rel_exptime = static_cast<int>(exptime - owner_->startTime());
        if (rel_exptime < 1) {
            rel_exptime = 1;
        }
    } else {
        // rel_exptime = exptime + currentTime;
    }

    if (good && policy_ == Item::kCas) {
        good = r.read(&cas);
    }

    // 格式错误。现在只允许 uint32_t、time_t、int、uint64_t这几种类型
    if (!good) {
        reply("CLIENT_ERROR bad command line format\r\n");
        return true;
    }

    // bytes超过限定值
    if (bytes > 1024 * 1024) {
        reply("SERVER_ERROR object too large for cache\r\n");
        needle_->resetKey(key);
        owner_->deleteItem(needle_);
        bytesToDiscard_ = bytes + 2;
        state_ = kDiscardValue; // 准备丢弃该命令
        return false;
    } else {
        currItem_ = Item::makeItem(key, flags, rel_exptime, bytes + 2, cas);    // 用item存储单个命令
        state_ = kReceiveValue; // 准备接收值
        return false;
    }
}

/**
 * memcache命令
 *  delete key [noreply]
 * 
 * 参数解释
 *  key：键值 key-value 结构中的 key，用于查找缓存值
 *  noreply（可选）： 该参数告知服务器不需要返回数据
 */
void Session::doDelete(Session::Tokenizer::iterator& beg, Session::Tokenizer::iterator end)
{
    assert(command_  == "delete");

    StringPiece key = *beg;
    bool good = key.size() <= kLongestKeySize;
    ++beg;

    if (!good) {
        reply("CLIENT_ERROR bad command line format\r\n");
    } else if (beg != end && *beg != "0") {
        reply("CLIENT_ERROR bad command line format, Usage: delete <key> [noreply]\r\n");
    } else {
        needle_->resetKey(key);
        if (owner_->deleteItem(needle_)) {
            reply("DELETED\r\n");
        } else {
            reply("NOT_FOUND\r\n");
        }
    }
}