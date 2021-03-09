#include "HttpContext.h"
#include "Buffer.h"
#include "assert.h"

void test_case_1()
{
    HttpContext context;
    Buffer input;
    input.append("GET /index.html HTTP/1.1\r\n" "Host: www.chenshuo.com\r\n" "\r\n");
    assert(context.parseRequest(&input, Timestamp::now()) == true);
    assert(context.gotAll() == true);
}

int main() {

    test_case_1();
    return 0;
}