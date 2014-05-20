#ifndef NG_PROFILER_HPP
#define NG_PROFILER_HPP

#include <chrono>

namespace ng
{

class Profiler
{
    std::chrono::milliseconds mTimeSpent = std::chrono::milliseconds(0);
    std::chrono::time_point<std::chrono::high_resolution_clock> mLastStart;
    int mNumSamples = 0;

public:
    void Reset();
    void Start();
    void Stop();

    int GetNumSamples() const;
    double GetTotalTimeMS() const;
    double GetAverageTimeMS() const;
};

} // end namespace ng

#endif // NG_PROFILER_HPP
