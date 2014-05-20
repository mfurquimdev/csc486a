#include "ng/engine/profiler.hpp"

namespace ng
{

void Profiler::Reset()
{
    mTimeSpent = std::chrono::milliseconds(0);
    mNumSamples = 0;
}

void Profiler::Start()
{
    mLastStart = std::chrono::high_resolution_clock::now();
}

void Profiler::Stop()
{
    mTimeSpent += std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - mLastStart);
    mNumSamples++;
}

double Profiler::GetTotalTimeMS() const
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(mTimeSpent).count();
}

double Profiler::GetAverageTimeMS() const
{
    return double(mTimeSpent.count()) / double(mNumSamples);
}

} // end namespace ng

