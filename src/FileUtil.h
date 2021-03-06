#ifndef EVENT_FILEUTIL_H
#define EVENT_FILEUTIL_H

#include <boost/noncopyable.hpp>
#include "StringPiece.h"

class AppendFile: boost::noncopyable
{
    private:
        size_t write(const char *logline, size_t len);
        FILE* fp_;
        char buffer_ [60 * 1024];
        off_t writtenBytes_;
    public:
        explicit AppendFile(std::string filename);

        ~AppendFile();

        // append 往文件中写
        void append(const char *logline, const size_t len);

        void flush();

        off_t writtenBytes() const
        {
            return writtenBytes_;
        }
};

class ReadSmallFile: boost::noncopyable
{
    public:
        static const int kBufferSize = 64 * 1024;

    private:
        int fd_;
        int err_;
        char buf_[kBufferSize];
    
    public:
        ReadSmallFile(StringArg filename);
        ~ReadSmallFile();

        template<typename String>
        int readToString(int maxSize, String* content, int64_t *fileSize, int64_t *modifyTime, int64_t *createTime);

        int readToBuffer(int *size);

        const char *buffer() const
        {
            return buf_;
        }

};

template<typename String>
int readFile(StringArg filename, 
    int maxSize, 
    String *content, 
    int64_t *fileSize = NULL, 
    int64_t *modifyTime = NULL, 
    int64_t *createTime = NULL)
{
    ReadSmallFile file(filename);
    return file.readToString(maxSize, content, fileSize, modifyTime, createTime);
}

#endif