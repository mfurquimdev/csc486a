#include "ng/engine/util/profiler.hpp"

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
    mTimeSpent += std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now() - mLastStart);
    mNumSamples++;
}

std::chrono::milliseconds::rep Profiler::GetTotalTimeMS() const
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(mTimeSpent).count();
}

std::chrono::milliseconds::rep Profiler::GetAverageTimeMS() const
{
    return mNumSamples == 0 ? 0 : GetTotalTimeMS() / mNumSamples;
}

} // end namespace ng

