#include "HttpContext.h"
#include "Buffer.h"
#include "assert.h"
#include <iostream>

void test_case_1()
{
    HttpContext context;
    Buffer input;
    input.append("GET /index.html HTTP/1.1\r\n" "Host: www.baidu.com\r\n" "\r\n");
    assert(context.parseRequest(&input, Timestamp::now()) == true);
    assert(context.gotAll() == true);

    const HttpRequest& request = context.request();
    assert(request.method() == HttpRequest::kGet);
    assert(request.path() == string("/index.html"));
    assert(request.getVersion() == HttpRequest::kHttp11);
    assert(request.getHeader("Host") == string("www.baidu.com"));
    assert(request.getHeader("User-Agent") == string(""));
}

void test_case_2()
{
    string all("GET /index.html HTTP/1.1\r\n" "Host: www.baidu.com\r\n" "\r\n");

    for (size_t sz1 = 0; sz1 < all.size(); ++sz1) {
        HttpContext context;
        Buffer input;
        input.append(all.c_str(), sz1);
        assert(context.parseRequest(&input, Timestamp::now()) == true);
        assert(context.gotAll() != true);

        size_t sz2 = all.size() - sz1;
        input.append(all.c_str() + sz1, sz2);
        assert(context.parseRequest(&input, Timestamp::now()) == true);
        assert(context.gotAll() == true);

        const HttpRequest& request = context.request();
        assert(request.method() == HttpRequest::kGet);
        assert(request.path() == string("/index.html"));
        assert(request.getVersion() == HttpRequest::kHttp11);
        assert(request.getHeader("Host") == string("www.baidu.com"));
        assert(request.getHeader("User-Agent") == string(""));
    }
}

void test_case_3()
{
    HttpContext context;
    Buffer input;
    input.append("GET /index.html HTTP/1.1\r\n" "Host: www.baidu.com\r\n" "User-Agent:\r\n" "Accept-Encodeing: \r\n" "\r\n");

    assert(context.parseRequest(&input, Timestamp::now()) == true);
    assert(context.gotAll() == true);

    const HttpRequest& request = context.request();
    assert(request.method() == HttpRequest::kGet);
    assert(request.path() == string("/index.html"));
    assert(request.getVersion() == HttpRequest::kHttp11);
    assert(request.getHeader("Host") == string("www.baidu.com"));
    assert(request.getHeader("User-Agent") == string(""));
    assert(request.getHeader("Accept-Encoding") == string(""));
}

int main() {

    test_case_3();
    return 0;
}