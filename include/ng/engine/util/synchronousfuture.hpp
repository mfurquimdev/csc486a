#ifndef NG_SYNCHRONOUSFUTURE_HPP
#define NG_SYNCHRONOUSFUTURE_HPP

#include "ng/engine/util/memory.hpp"

#include <future>

// An implementation of std::future/std::promise that is not multi-threaded.
// For use on platforms where std::future/std::promise are not available
// As opposed to normal futures and promises, these ones do not actually allow anything to be asynchronous
// That is, the result of the promise must always be already available when the future asks for it.
// It's really meant as a compatibility thing for code that is can be used asynchronously depending on the platform.
// For example, depending on the platform, there could be a different typedef for my_future_type using either std::future or ng::synchronous_future.

// the current main use case is because clang/emscripten has problems with exception_ptr (of all things) in its std::future ("exception_ptr not yet implemented")
// WARNING: this is currently a really shitty hack job to get emscripten going tonight.
//          it is not up to spec with future/promise, and it's certainly not as efficient as possible.

namespace ng
{

// Forward declarations.
template<typename T>
class synchronous_future;

template<typename T>
class synchronous_shared_future;

template<typename T>
class synchronous_promise;

template<class T>
class synchronous_future
{
private:
    std::shared_ptr<std::unique_ptr<T>> mData;

public:
    friend class synchronous_shared_future<T>;
    friend class synchronous_promise<T>;

    bool valid() const
    {
        return mData != nullptr;
    }

    void wait() const
    {
        if (valid() == false)
        {
            throw std::future_error(std::future_errc::no_state);
        }
    }

    synchronous_future() = default;

    synchronous_future(synchronous_future&& other)
    {
        swap(mData, other.mData);
        other.mData.reset();
    }

    synchronous_future(const synchronous_future& other) = delete;

    synchronous_future& operator=(synchronous_future&& other)
    {
        if (this != &other)
        {
            swap(mData, other.mData);
            other.mData.reset();
        }

        return *this;
    }

    synchronous_future& operator=(const synchronous_future& other) = delete;

    synchronous_shared_future<T> share();

    T get()
    {
        wait();
        return std::move(**mData);
    }
};

template<class T>
class synchronous_shared_future
{
    std::shared_ptr<std::unique_ptr<T>> mData;

public:
    synchronous_shared_future() = default;

    synchronous_shared_future(synchronous_future<T>&& other)
    {
        swap(mData, other.mData);
        other.mData.reset();
    }

    synchronous_shared_future& operator=(synchronous_future<T>&& other)
    {
        swap(mData, other.mData);
        other.mData.reset();
        return *this;
    }

    bool valid() const
    {
        return mData != nullptr;
    }

    void wait() const
    {
        if (valid() == false)
        {
            throw std::future_error(std::future_errc::no_state);
        }
    }

    const T& get() const
    {
        wait();
        return **mData;
    }
};

template<class T>
synchronous_shared_future<T> synchronous_future<T>::share()
{
    wait();
    return { *this };
}

template<class T>
class synchronous_promise
{
private:
    std::shared_ptr<std::unique_ptr<T>> mData;

public:
    synchronous_promise()
    {
        mData.reset(new std::unique_ptr<T>);
    }

    synchronous_promise(synchronous_promise&& other) = default;
    synchronous_promise(const synchronous_promise& other) = delete;

    synchronous_promise& operator=(synchronous_promise&& other) = default;
    synchronous_promise& operator=(const synchronous_promise& other) = delete;

    void swap(synchronous_promise& other)
    {
        swap(mData, other.mData);
    }

    synchronous_future<T> get_future()
    {
        synchronous_future<T> f;
        f.mData = mData;
        return f;
    }

    void set_value(const T& value)
    {
        if (mData == nullptr)
        {
            throw std::future_error(std::future_errc::no_state);
        }

        if (*mData != nullptr)
        {
            throw std::future_error(std::future_errc::promise_already_satisfied);
        }

        *mData = ng::make_unique<T>(value);
    }

    void set_value(T&& value)
    {
        if (mData == nullptr)
        {
            throw std::future_error(std::future_errc::no_state);
        }

        if (*mData != nullptr)
        {
            throw std::future_error(std::future_errc::promise_already_satisfied);
        }

        *mData = ng::make_unique<T>(std::move(value));
    }
};

} // end namespace ng

#endif // NG_SYNCHRONOUSFUTURE_HPP
