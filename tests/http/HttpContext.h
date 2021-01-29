#ifndef EVENT_TEST_HTTPCONTEXT_H
#define EVENT_TEST_HTTPCONTEXT_H

class HttpContext
{
    public:
        enum HttpRequestParseState
        {
            kExpectRequestLine,
            kExpectHeaders,
            kExpectBody,
            kGotAll
        };

        HttpContext(): state_(kExpectRequestLine)
        {

        }

        
};

#endif