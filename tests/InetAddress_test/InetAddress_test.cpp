#include "InetAddress.h"
#include <assert.h>

using std::string;

void test_case_1() 
{
    InetAddress addr0(1234);
    assert(addr0.toIp() == string("0.0.0.0"));
    assert(addr0.toIpPort() == string("0.0.0.0:1234"));
    assert(addr0.toPort() == 1234);

    InetAddress addr1(4321, true);
    assert(addr1.toIp() == string("127.0.0.1"));
    assert(addr1.toIpPort() == string("127.0.0.1:4321"));
    assert(addr1.toPort() == 4321);

    InetAddress addr2("1.2.3.4", 8888);
    assert(addr2.toIp() == string("1.2.3.4"));
    assert(addr2.toIpPort() == string("1.2.3.4:8888"));
    assert(addr2.toPort() == 8888);

    InetAddress addr3("255.254.253.252", 65535);
    assert(addr3.toIp() == string("255.254.253.252"));
    assert(addr3.toIpPort() == string("255.254.253.252:65535"));
    assert(addr3.toPort() == 65535);
}

void test_case_2()
{
    InetAddress addr(80);
    if (InetAddress::resolve("google.com", &addr)) {
        printf("google.com resolved to %s\n", addr.toIpPort().c_str());
    } else {
        printf("Unable to resolve google.com\n");
    }
}

int main() {

    test_case_2();
    return 0;
}