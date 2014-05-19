#ifndef NG_RESOURCE_HPP
#define NG_RESOURCE_HPP

#include <memory>
#include <cstdint>

namespace ng
{

struct ResourceHandle
{
    std::uint32_t ID;
    std::uint32_t Class;
    std::shared_ptr<void> Data;

    ResourceHandle(std::uint32_t id,
                   std::uint32_t clazz,
                   std::shared_ptr<void> data)
        : ID(id)
        , Class(clazz)
        , Data(std::move(data))
    { }

    template<class T>
    T* GetData()
    {
        return static_cast<T*>(Data.get());
    }

    template<class T>
    const T* GetData() const
    {
        return static_cast<const T*>(Data.get());
    }
};

} // end namespace ng

#endif // NG_RESOURCE_HPP
