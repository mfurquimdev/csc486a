#ifndef NG_RESOURCE_HPP
#define NG_RESOURCE_HPP

#include <memory>
#include <cstdint>

namespace ng
{

struct ResourceHandle
{
    using InstanceID = std::uint64_t;

    union ClassID
    {
        std::uint32_t AsUInt;
        char AsBytes[4];

        ClassID() = default;

        explicit constexpr ClassID(std::uint32_t asUInt)
            : AsUInt(asUInt)
        { }

        template<std::size_t N>
        explicit constexpr ClassID(const char (&fourLetterCode)[N])
            : AsBytes {
                  fourLetterCode[0],
                  fourLetterCode[1],
                  fourLetterCode[2],
                  fourLetterCode[3] }
        {
            static_assert(N >= 4, "should be passing in a four-letter code.");
        }
    };

    InstanceID Instance;
    ClassID Class;

    std::shared_ptr<void> Data;

    ResourceHandle()
        : Instance(0)
        , Class(0)
    { }

    ResourceHandle(std::nullptr_t)
        : ResourceHandle()
    { }

    template<class T>
    ResourceHandle(InstanceID instance,
                   ClassID clazz,
                   std::shared_ptr<T> data)
        : Instance(instance)
        , Class(clazz)
        , Data(std::move(data))
    { }

    template<class T>
    T* GetPtr()
    {
        return static_cast<T*>(Data.get());
    }

    template<class T>
    const T* GetPtr() const
    {
        return static_cast<const T*>(Data.get());
    }
};

} // end namespace ng

#endif // NG_RESOURCE_HPP
