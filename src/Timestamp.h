#ifndef EVENT_TIMESTAMP_H
#define EVENT_TIMESTAMP_H

#include <stdint.h>
#include "Types.h"

class Timestamp
{
    private:
        int64_t microSecondsSinceEpoch_;
    
    public:
        static const int kMicroSecondsPerSecond = 1000 * 1000;
    
    public:
        Timestamp(): microSecondsSinceEpoch_(0)
        {

        }

        explicit Timestamp(int64_t microSecondsSinceEpochArg): microSecondsSinceEpoch_(microSecondsSinceEpochArg)
        {

        }

        void swap(Timestamp& that)
        {
            std::swap(microSecondsSinceEpoch_, that.microSecondsSinceEpoch_);
        }

        std::string toString() const;
        std::string toFormattedString(bool showMicroseconds = true) const;

        bool valid() const
        {
            return microSecondsSinceEpoch_ > 0;
        }

        int64_t microSecondsSinceEpoch() const
        {
            return microSecondsSinceEpoch_;
        }

        time_t secondsSinceEpoch() const
        {
            return static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
        }

        static Timestamp now();

        static Timestamp invalid()
        {
            return Timestamp();
        }

        static Timestamp fromUnixTime(time_t t, int microseconds)
        {
            return Timestamp(static_cast<int64_t>(t) * kMicroSecondsPerSecond + microseconds);
        }
};

inline bool operator<(Timestamp lhs, Timestamp rhs)
{
    return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
}

inline bool operator==(Timestamp lhs, Timestamp rhs)
{
    return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
}

inline double timeDifference(Timestamp high, Timestamp low)
{
    int64_t diff = high.microSecondsSinceEpoch() - low.microSecondsSinceEpoch();
    return static_cast<double>(diff) / Timestamp::kMicroSecondsPerSecond;
}

inline Timestamp addTime(Timestamp timestamp, double seconds)
{
    int64_t delta = static_cast<int64_t>(seconds * Timestamp::kMicroSecondsPerSecond);
    return Timestamp(timestamp.microSecondsSinceEpoch() + delta);
}

#endif