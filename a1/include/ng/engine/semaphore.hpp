#ifndef NG_SEMAPHORE_HPP
#define NG_SEMAPHORE_HPP

#include <mutex>
#include <condition_variable>

namespace ng
{

class semaphore
{
    std::mutex mMutex;
    std::condition_variable mCondition;
    unsigned long mCount;

public:
    semaphore(unsigned long initialCount = 0)
        : mCount(initialCount)
    { }

    void post()
    {
        std::lock_guard<std::mutex> lock(mMutex);
        ++mCount;
        mCondition.notify_one();
    }

    void wait()
    {
        std::unique_lock<std::mutex> lock(mMutex);
        while (mCount == 0)
        {
            mCondition.wait(lock);
        }
        --mCount;
    }
};

} // end namespace ng

#endif // NG_SEMAPHORE_HPP
