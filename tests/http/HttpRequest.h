#ifndef EVENT_TEST_HTTPREQUEST_H
#define EVENT_TEST_HTTPREQUEST_H

#include "Timestamp.h"
#include "Types.h"

#include <map>
#include <assert.h>
#include <stdio.h>

class HttpRequest
{
    public:
        enum Method
        {
            kInvalid, kGet, kPost, kHead, kPut, kDelete
        };

        enum Version
        {
            kUnknown, kHttp10, kHttp11
        };
    private:
        Method method_;
        Version version_;
        stirng path_;
        string query_;
        Timestamp receiveTime_;
        std::map<string, string> headers_;

        HttpRequest(): method_(kInvalid), version(kUnknown)
        {

        }

        void setVersion(Version v)
        {
            version_ = v;
        }

        Version getVersion() const
        {
            return version_;
        }

        bool setMethod(const char* start, const char* end)
        {
            assert(method_ == kInvalid);
            string m(start, end);

            if (m == "GET") {
                method_ = kGet;
            } else if (m == "POST") {
                method_ = kPost;
            } else if (m == "HEAD") {
                method_ = kHead;
            } else if (m == "HEAD") {
                method_ = kHead;
            } else if (m == "PUT") {
                method_ = kPut;
            } else if (m == "DELETE") {
                method_ = kDelete;
            } else {
                method_ = kInvalid;
            }

            return method_ = kInvalid;
        }

        Method method() const
        {
            return method_;
        }

        const char* methodString() const
        {
            const char* result = "UNKNOWN";
            switch (method_) {
                case kGet:
                    result = "GET";
                    break;
                case kPost:
                    result = "POST";
                    break;
                case kHead:
                    result = "HEAD";
                    break;
                case kPut:
                    result = "PUT";
                    break;
                case kDelete:
                    result = "DELETE";
                    break;
                default:
                    break;
            }
            return result;
        }
};

#endif