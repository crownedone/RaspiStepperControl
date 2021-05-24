#pragma once

#if defined(BOOST_WINDOWS)
    #include <cstdint>
#else
    #include <chrono>
#endif

/// Stop watch.
/// @ingroup ComponentGroup
class StopWatch
{

private:
// MSVC does not have a high_resolution_clock in chrono until VS2015
#if defined(BOOST_WINDOWS)
    /// First time point.
    int64_t mStart;
#else
    /// First time point.
    std::chrono::high_resolution_clock::time_point mStart;
#endif

public:
    /// Constructor.
    /// Start is automatically called.
    StopWatch();

    /// Start stop watch.
    void start();

    /// Stop and return time in milliseconds.
    /// @return Time in milliseconds.
    double stop();

    /// Restarts timer and returns time until now.
    /// @return Time in milliseconds.
    double restart();
};
