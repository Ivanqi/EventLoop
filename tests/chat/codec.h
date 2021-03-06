#ifndef EVENT_TEST_CODE_H
#define EVENT_TEST_CODE_H

#include "Buffer.h"
#include "Endian.h"
#include "TcpConnection.h"

class LengthHeaderCodec
{
    public:
        typedef std::function<void (const TcpConnectionPtr&, string& message, Timestamp)> StringMessageCallback;

    private:
        StringMessageCallback messageCallback_;
        const static size_t kHeaderLen = sizeof(int32_t);

    public:
        LengthHeaderCodec(const StringMessageCallback& cb)
            : messageCallback_(cb)
        {

        }

        void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp receiveTime)
        {
            while (buf->readableBytes() >= kHeaderLen) {    // kHeaderLen == 4
                const void *data = buf->peek();             // 到buffer begin + readIndex_的数据
                int32_t be32 = *static_cast<const int32_t*>(data);
                const int32_t len = networkToHost32(be32);
                printf("message:%d\n", len);
                if (len > 65536 || len < 0) {
                    printf("Invalid length %d\n", (int)len);
                    conn->shutdown();
                    break;
                } else if (buf->readableBytes() >= len + kHeaderLen) {
                    buf->retrieve(kHeaderLen);
                    string message(buf->peek(), len);
                    messageCallback_(conn, message, receiveTime);
                    buf->retrieve(len);
                } else {
                    break;
                }
            }
           
        }
        /**
         * 这段代码把 string message 打包为Buffer，并通过conn发送
         */
        void send(TcpConnection *conn, const StringPiece& message)
        {
            Buffer buf;
            buf.append(message.data(), message.size());
            int32_t len = static_cast<int32_t>(message.size());
            int32_t be32 = hostToNetwork32(len);
            buf.prepend(&be32, sizeof(be32));
            conn->send(&buf);
        }
};

#endif