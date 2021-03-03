#include "Buffer.h"
#include "assert.h"
#include <iostream>
using namespace std;

void test_case_1()
{
    Buffer buf;
    
    assert(buf.readableBytes() == 0);
    assert(buf.writableBytes() == Buffer::kInitialSize);
    assert(buf.prependableBytes() == Buffer::kCheapPrepend);

    const string str(200, 'x');
    buf.append(str);
    assert(buf.readableBytes() == str.size());
    assert(buf.writableBytes() == Buffer::kInitialSize - str.size());
    assert(buf.prependableBytes() == Buffer::kCheapPrepend);

    const string str2 = buf.retrieveAsString(50);
    assert(str2.size() == 50);
    assert(buf.readableBytes() == (str.size() - str2.size()));
    assert(buf.writableBytes() == Buffer::kInitialSize - str.size());
    assert(buf.prependableBytes() == (Buffer::kCheapPrepend + str2.size()));
    assert(str2 == string(50, 'x'));

    buf.append(str);
    assert(buf.readableBytes() == (2 * str.size() - str2.size()));
    assert(buf.writableBytes() == (Buffer::kInitialSize - 2 * str.size()));
    assert(buf.prependableBytes() == (Buffer::kCheapPrepend + str2.size()));

    const string str3 = buf.retrieveAllAsString();
    assert(str3.size() == 350);
    assert(buf.readableBytes() == 0);
    assert(buf.writableBytes() == Buffer::kInitialSize);
    assert(buf.prependableBytes() == Buffer::kCheapPrepend);
    assert(str3 == string(350, 'x'));
}

int main() {

    test_case_1();
    return 0;
}