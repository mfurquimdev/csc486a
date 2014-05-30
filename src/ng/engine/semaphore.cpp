#include "ng/engine/util/semaphore.hpp"

namespace ng
{

semaphore::semaphore(unsigned long initialCount)
    : mCount(initialCount)
{ }

void semaphore::post()
{
    std::lock_guard<std::mutex> lock(mMutex);
    ++mCount;
    mCondition.notify_one();
}

void semaphore::wait()
{
    std::unique_lock<std::mutex> lock(mMutex);
    while (mCount == 0)
    {
        mCondition.wait(lock);
    }
    --mCount;
}

int semaphore::getvalue() const
{
    return mCount;
}

} // end namespace ng
