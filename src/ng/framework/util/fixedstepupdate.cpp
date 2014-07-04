#include "ng/framework/util/fixedstepupdate.hpp"

#include <stdexcept>

namespace ng
{

FixedStepUpdate::FixedStepUpdate(std::chrono::milliseconds stepDuration)
    : mStepDuration(stepDuration > std::chrono::milliseconds(0) ?
                        stepDuration
                      : throw std::logic_error(
                            "stepDuration must be > 0 milliseconds"))
    , mLag(0)
{ }

std::chrono::milliseconds FixedStepUpdate::GetStepDuration() const
{
    return mStepDuration;
}

void FixedStepUpdate::QueuePendingSteps()
{
    mTimeOfThisStep = TimePoint::clock::now();

    // in case this is the first update
    if (mTimeOfLastStep == TimePoint())
    {
        mTimeOfLastStep = mTimeOfThisStep;
    }

    std::chrono::milliseconds dt =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                mTimeOfThisStep - mTimeOfLastStep);

    mTimeOfLastStep = mTimeOfThisStep;

    mLag += dt;
}

int FixedStepUpdate::GetNumPendingSteps() const
{
    return mLag / mStepDuration;
}

void FixedStepUpdate::Step()
{
    if (mLag < mStepDuration)
    {
        throw std::logic_error("Can't Step() when GetNumPendingSteps() == 0");
    }

    mLag -= mStepDuration;
}

} // end namespace ng
