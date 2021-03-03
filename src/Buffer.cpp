#include "Buffer.h"
#include "SocketsOps.h"

#include <errno.h>
#include <sys/uio.h>

const char Buffer::kCRLF[] = "\r\n";

const size_t Buffer::kCheapPrepend;
const size_t Buffer::kInitialSize;

/**
 * 1. 使用了 scatter / gather IO，并且一部分缓冲区取自stack，这样输入缓冲区足够大，也节省了一次ioctl(socketFd, FIONREAD, &length)系统调用
 * 2. Buffer::readFd()只调用一次read(2)，而没有反复调用read(2)直到返回EAGAIN
 * 
 * 具体流程
 *  1. 在栈上准备一个65535字节的extrabuf，然后利用readv()来读取数据
 *  2. iovec有两块
 *      1. 第一块指向Buffer中的writable字节
 *      2. 另一块指向栈上的extrabuf
 *  3. 如果读入的数据不多，那么全部都读到Buffer中去
 *  4. 如果长度超过Buffer的writealbe字节数，就会读到栈上的extrabuf里，然后再把extrabuf里的数据append()到Buffer中去
 * 
 * 好处:
 *  1. 这么做利用了临时栈上的空间，避免每个连接的初始Buffer过大造成的内存浪费
 *  2. 也避免反复调用read()的系统开销(由于缓冲区足够大，通常一次readv()系统调用就能读全部数据)
 *  3. 由于网络哭使用的是level trigger，因此这个函数并不会反复调用read()直到返回EAGAIN,从而可以降低消息处理的延迟
 */
ssize_t Buffer::readFd(int fd, int *saveErrno)
{
    // 保存了一个ioctl()/FIONREAD 调用来指示要读取多少
    char extrabuf[65536];
    struct iovec vec[2];
    const size_t writable = writableBytes();

    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);

    /**
     * 当缓冲区中有足够的空间时，不要读入extrabuf
     * 当使用extrabuf时，我们最多读取128k-1字节
     */
    const int iovcnt = (writable < sizeof(extrabuf)) ? 2 : 1;
    const ssize_t n = sockets::readv(fd, vec, iovcnt);

    if (n < 0) {
        *saveErrno = errno;
    } else if (implicit_cast<size_t>(n) <= writable) {
        writerIndex_ += n;
    } else {
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writable);
    }

    return n;
}