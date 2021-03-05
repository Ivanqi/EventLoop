#include "Buffer.h"
#include "assert.h"
#include <utility>
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

void test_case_2() {
    Buffer buf;
    buf.append(string(400, 'y'));
    assert(buf.readableBytes() == 400);
    assert(buf.writableBytes() == Buffer::kInitialSize - 400);

    buf.retrieve(50);
    assert(buf.readableBytes() == 350);
    assert(buf.writableBytes() == Buffer::kInitialSize - 400);
    assert(buf.prependableBytes() == Buffer::kCheapPrepend + 50);


    buf.append(string(1000, 'z'));
    assert(buf.readableBytes() == 1350);
    assert(buf.writableBytes() != 1400);    // 超出原有的内存，进行内存扩展。扩展的内存和写入的内容所占内存刚好一致
    assert(buf.prependableBytes() == (Buffer::kCheapPrepend + 50));

    buf.retrieveAll();
    assert(buf.readableBytes() == 0);
    assert(buf.writableBytes() == 1400);
    assert(buf.prependableBytes() == Buffer::kCheapPrepend);
}

void test_case_3() {
    Buffer buf;
    buf.append(string(800, 'y'));
    assert(buf.readableBytes() == 800);
    assert(buf.writableBytes() == Buffer::kInitialSize - 800);

    buf.retrieve(500);
    assert(buf.readableBytes() == 300);
    assert(buf.writableBytes() == Buffer::kInitialSize - 800);
    assert(buf.prependableBytes() == Buffer::kCheapPrepend + 500);

    buf.append(string(300, 'z'));
    assert(buf.readableBytes() == 600);
    assert(buf.writableBytes() == Buffer::kInitialSize - 600);
    assert(buf.prependableBytes() == Buffer::kCheapPrepend);
}

void test_case_4() {
    Buffer buf;
    buf.append(string(2000, 'y'));
    assert(buf.readableBytes() == 2000);
    assert(buf.writableBytes() == 0);
    assert(buf.prependableBytes() == Buffer::kCheapPrepend);

    buf.retrieve(1500);
    assert(buf.readableBytes() == 500);
    assert(buf.writableBytes() == 0);
    assert(buf.prependableBytes() == Buffer::kCheapPrepend + 1500);

    buf.shrink(0);
    assert(buf.readableBytes() == 500);
    assert(buf.writableBytes() == Buffer::kInitialSize - 500);
    assert(buf.retrieveAllAsString() == string(500, 'y'));
    assert(buf.prependableBytes() == Buffer::kCheapPrepend);
}

void test_case_5() {
    Buffer buf;
    buf.append(string(200, 'y'));
    assert(buf.readableBytes() == 200);
    assert(buf.writableBytes() == Buffer::kInitialSize - 200);
    assert(buf.prependableBytes() == Buffer::kCheapPrepend);

    int x = 0;
    buf.prepend(&x, sizeof(x));
    assert(buf.readableBytes() == 204);
    assert(buf.writableBytes() == Buffer::kInitialSize - 200);
    assert(buf.prependableBytes() == Buffer::kCheapPrepend - 4);
}

void test_case_6() {
    Buffer buf;
    buf.append("HTTP");

    assert(buf.readableBytes() == 4);
    assert(buf.peekInt8() == 'H');

    int top16 = buf.peekInt16();
    assert(top16 == ('H' * 256 + 'T'));
    assert(buf.peekInt32() != (top16 * 65535 + 'T' * 256 + 'P'));

    assert(buf.readInt8() == 'H');
    assert(buf.readInt16() == ('T' * 256 + 'T'));
    assert(buf.readInt8() == 'P');
    assert(buf.readableBytes() == 0);
    assert(buf.writableBytes() == Buffer::kInitialSize);

    buf.appendInt8(-1);
    buf.appendInt16(-2);
    buf.appendInt32(-3);
    assert(buf.readableBytes() == 7);
    assert(buf.readInt8() == -1);
    assert(buf.readInt16() == -2);
    assert(buf.readInt32() == -3);
}

void test_case_7() {
    Buffer buf;
    buf.append(string(100000, 'x'));
    const char* null = NULL;
    assert(buf.findEOL() == null);
    assert(buf.findEOL(buf.peek() + 90000) == null);
}

void output(Buffer&& buf, const void* inner) {
    Buffer newbuf(std::move(buf));
    assert(inner == newbuf.peek());
}

void test_case_8() {
    Buffer buf;
    buf.append("ivan", 4);
    const void *inner = buf.peek();
    output(move(buf), inner);
}

int main() {

    test_case_8();
    return 0;
}