#include "StopWatch.hpp"

#if defined(BOOST_WINDOWS)
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include "Windows.h"

static LARGE_INTEGER GetQueryPerformanceFrequency()
{
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    return freq;
}

static LARGE_INTEGER FREQUENCY = GetQueryPerformanceFrequency();
#endif


StopWatch::StopWatch()
{
    start();
}

void StopWatch::start()
{
#if defined(BOOST_WINDOWS)
    LARGE_INTEGER l;
    QueryPerformanceCounter(&l);
    mStart = l.QuadPart;
#else
    mStart = std::chrono::high_resolution_clock::now();
#endif
}

double StopWatch::restart()
{
    double milliseconds = 0;

#if defined(BOOST_WINDOWS)
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    milliseconds = static_cast<float>(static_cast<double>(now.QuadPart - mStart) /
                                      (static_cast<double>(FREQUENCY.QuadPart)));
    milliseconds *= 1000;
    mStart = now.QuadPart;
#else
    auto now = std::chrono::high_resolution_clock::now();
    milliseconds =
        static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(now - mStart)
                            .count());
    milliseconds /= 1000;
    mStart = now;
#endif

    return milliseconds;
}

double StopWatch::stop()
{
    auto start = mStart;

    auto milliseconds = restart();

    mStart = start;

    return milliseconds;
}
