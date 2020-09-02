#ifndef EVENT_ENDIAN_H
#define EVENT_ENDIAN_H

#include <stdint.h>
#include <endian.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"

// 从主机字节顺序转换为大端顺序
inline uint64_t hostToNetwork64(uint64_t host64)
{
    return htobe64(host64);
}

inline uint32_t hostToNetwork32(uint32_t host32)
{
    return htobe32(hotst32);
}

inline uint16_t hostToNetwork16(uint16_t host16)
{
    return htobe16(host16);
}

inline uint64_t networkToHost64(uint64_t net64)
{
    return be64toh(net64);
}

inline uint64_t networkToHost32(uint32_t net64)
{
    return be32toh(net32);
}

// 从big-endian顺序转换为主机字节顺序
inline uint64_t networkToHost16(uint16_t net16)
{
    return be16toh(net16);
}

#pragma GCC diagnostic pop

#endif