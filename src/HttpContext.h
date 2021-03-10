#ifndef EVENT_HTTPCONTEXT_H
#define EVENT_HTTPCONTEXT_H

#include "HttpRequest.h"

class Buffer;
class HttpContext
{
    public:
        enum HttpRequestParseState
        {
            kExpectRequestLine, // 请求行
            kExpectHeaders, // 请求头
            kExpectBody,    // 请求体
            kGotAll // 已经获取所有信息
        };

    private:
        HttpRequestParseState state_;
        HttpRequest request_;
    
    public:
        HttpContext()
            :state_(kExpectRequestLine)
        {
        }

        bool parseRequest(Buffer *buf, Timestamp receiveTime);

        bool gotAll() const
        {
            return state_ == kGotAll;
        }

        void reset()
        {
            state_ = kExpectRequestLine;
            HttpRequest dummy;
            request_.swap(dummy);
        }

        const HttpRequest& request() const
        {
            return request_;
        }

        HttpRequest& request()
        {
            return request_;
        }
    
    private:
        bool processRequestLine(const char* begin, const char* end);
};

#endif