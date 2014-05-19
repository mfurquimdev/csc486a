#ifndef NG_MEMORY_HPP
#define NG_MEMORY_HPP

#include <memory>

namespace ng
{

template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

} // end namespace ng

#endif // NG_MEMORY_HPP
