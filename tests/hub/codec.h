#ifndef EVENT_TEST_CODEC_H
#define EVENT_TEST_CODEC_H

#include "Types.h"
#include "Buffer.h"

using std::string;

enum ParseResult
{
    kError,
    kSuccess,
    kContinue,
};

ParseResult parseMessage(Buffer *buf, string *cmd, string *topic, string *content);

#endif