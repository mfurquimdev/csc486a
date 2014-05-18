#ifndef NG_MEMORY_HPP
#define NG_MEMORY_HPP

#include <memory>
#include <functional>

namespace ng
{

template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

// type erases the Deleter template argument of unique_ptr
template<class T>
using unique_deleted_ptr = std::unique_ptr<T, std::function<void(T*)>>;

} // end namespace ng

#endif // NG_MEMORY_HPP
