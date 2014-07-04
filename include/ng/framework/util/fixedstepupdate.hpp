#ifndef NG_FIXEDSTEPUPDATE_HPP
#define NG_FIXEDSTEPUPDATE_HPP

#include <chrono>

namespace ng
{

class FixedStepUpdate
{
public:
     using TimePoint = std::chrono::time_point
                      <std::chrono::high_resolution_clock>;

    FixedStepUpdate(std::chrono::milliseconds stepDuration);

    std::chrono::milliseconds GetStepDuration() const;

    // Call this once at the start of your main loop
    void QueuePendingSteps();

    // Get number of times you should update your app.
    // Each update should move the timeline forward
    // by GetStepDuration() milliseconds.
    int GetNumPendingSteps() const;

    // Call this each time you hae updated your app
    // by GetStepDuration milliseconds.
    void Step();

private:
     TimePoint mTimeOfLastStep, mTimeOfThisStep;
     std::chrono::milliseconds mStepDuration;
     std::chrono::milliseconds mLag;
};

} // end namespace ng

#endif // NG_FIXEDSTEPUPDATE_HPP
