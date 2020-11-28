
#ifndef MUDUO_BASE_TIMESTAMP_H
#define MUDUO_BASE_TIMESTAMP_H

#include "muduo/base/copyable.h"
#include "muduo/base/Types.h"

#include <boost/operators.hpp>

namespace muduo {

///
/// Time stamp in UTC, in microseconds resolution.
///
/// This class is immutable.
/// It's recommended to pass it by value, since it's passed in register on x64.
/// 主要是实现了时间戳的相关操作，例如时间戳的格式化，返回当前时间戳等等。

// boost::equality_comparable 只要实现对operator==就会自动实现!= ,一种模板元编程思想
// boost::less_than_comparable 要求我们实现 < ,然后就可自动实现>,<=,>=
class Timestamp : public muduo::copyable,
                  public boost::equality_comparable<Timestamp>,  
                  public boost::less_than_comparable<Timestamp> {
public:
    ///
    /// Constucts an invalid Timestamp.
    ///
    Timestamp() : microSecondsSinceEpoch_(0) { }

    ///
    /// Constucts a Timestamp at specific time
    ///
    /// @param microSecondsSinceEpoch
    explicit Timestamp(int64_t microSecondsSinceEpochArg)
      : microSecondsSinceEpoch_(microSecondsSinceEpochArg) { }
    
    // 两个时间戳之间进行交换，引用，所以能改变
    void swap(Timestamp& that) {
        std::swap(microSecondsSinceEpoch_, that.microSecondsSinceEpoch_);
    }

    // default copy/assignment/dtor are Okay

    string toString() const;
    string toFormattedString(bool showMicroseconds = true) const;

    bool valid() const { return microSecondsSinceEpoch_ > 0; }

    // for internal usage.
    int64_t microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; }
    time_t secondsSinceEpoch() const
    { return static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond); }

    ///
    /// Get time of now.
    ///
    static Timestamp now();
    static Timestamp invalid() {
      return Timestamp();
    }

    static Timestamp fromUnixTime(time_t t) {
      return fromUnixTime(t, 0);
    }

    static Timestamp fromUnixTime(time_t t, int microseconds) {
      return Timestamp(static_cast<int64_t>(t) * kMicroSecondsPerSecond + microseconds);
    }

    static const int kMicroSecondsPerSecond = 1000 * 1000;  // 1s = 100w ms

private:
    int64_t microSecondsSinceEpoch_;  // 距离1970年1月1日 的微秒
};

inline bool operator<(Timestamp lhs, Timestamp rhs) {
    return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
}

inline bool operator==(Timestamp lhs, Timestamp rhs) {
    return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
}

///
/// Gets time difference of two timestamps, result in seconds.
///
/// @param high, low
/// @return (high-low) in seconds
/// @c double has 52-bit precision, enough for one-microsecond
/// resolution for next 100 years.
inline double timeDifference(Timestamp high, Timestamp low) {
    int64_t diff = high.microSecondsSinceEpoch() - low.microSecondsSinceEpoch();
    return static_cast<double>(diff) / Timestamp::kMicroSecondsPerSecond;  // ms -> s
}

///
/// Add @c seconds to given timestamp.
///
/// @return timestamp+seconds as Timestamp
///
inline Timestamp addTime(Timestamp timestamp, double seconds) {
    int64_t delta = static_cast<int64_t>(seconds * Timestamp::kMicroSecondsPerSecond); // s -> ms
    return Timestamp(timestamp.microSecondsSinceEpoch() + delta);
}

}  // namespace muduo

#endif  // MUDUO_BASE_TIMESTAMP_H
