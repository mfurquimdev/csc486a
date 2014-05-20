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
    semaphore(unsigned long initialCount = 0);

    void post();

    void wait();
};

} // end namespace ng

#endif // NG_SEMAPHORE_HPP
