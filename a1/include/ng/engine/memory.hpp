#ifndef NG_MEMORY_HPP
#define NG_MEMORY_HPP

#include <memory>
#include <functional>

namespace ng
{

// type erases the Deleter template argument of unique_ptr
template<class T>
using unique_deleted_ptr = std::unique_ptr<T, std::function<void(T*)>>;

} // end namespace ng

#endif // NG_MEMORY_HPP
