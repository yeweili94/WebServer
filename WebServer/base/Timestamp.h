#ifndef WEB_SERVER_TIME_STAMP_H
#define WEB_SERVER_TIME_STAMP_H

#include <boost/operators.hpp>

#include <utility>

namespace ywl
{

template<typename T>
void swap(T& a, T& b)
{
    T c(std::move(a));
    a = std::move(b);
    b = std::move(c);
}

class Timestamp : public boost::less_than_comparable<Timestamp>
{
public:
    Timestamp()
        : microSecondsSinceEpoch_(0)
    {
    }

    explicit Timestamp(int64_t microSecondsSinceEpoch);

    void swap(Timestamp& that)
    {
        std::swap(microSecondsSinceEpoch_, that.microSecondsSinceEpoch_);
    }

    std::string toString() const;
    std::string toFormattedString() const;

    bool valid() const { return microSecondsSinceEpoch_ > 0; }

    int64_t microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; }
    time_t secondSinceEpoch() const
    {
        return static_cast<time_t>(microSecondsSinceEpoch_ / KMicroSecondsPerSecond);
    }

    static Timestamp now();
    static Timestamp invalid();

    static const int KMicroSecondsPerSecond = 1000 * 1000;
private:
    int64_t microSecondsSinceEpoch_;
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
    return static_cast<double>(diff) / Timestamp::KMicroSecondsPerSecond;
}

inline Timestamp addTime(Timestamp timestamp, double seconds)
{
    int64_t delta = static_cast<int64_t>(seconds * Timestamp::KMicroSecondsPerSecond);
    return Timestamp(timestamp.microSecondsSinceEpoch() + delta);
}

}//namespace ywl

#endif
