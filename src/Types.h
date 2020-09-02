#ifndef EVENT_TYPES_H
#define EVENT_TYPES_H

#include <stdint.h>
#include <string.h>
#include <string>

using std::string;

inline void memZero(void *p, size_t n) 
{
    memset(p, 0, n);
}

// implicit_cast只允许向下转型
template<typename To, typename From>
inline To implicit_cast(From const &f)
{
    return f;
}

#endif