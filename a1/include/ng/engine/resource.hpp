#ifndef NG_RESOURCE_HPP
#define NG_RESOURCE_HPP

#include <memory>
#include <cstdint>

namespace ng
{

struct ResourceHandle
{
    using IDType = std::uint32_t;

    union ClassIDType
    {
        std::uint32_t AsUInt;
        char AsBytes[4];

        ClassIDType() = default;

        explicit constexpr ClassIDType(std::uint32_t asUInt)
            : AsUInt(asUInt)
        { }

        template<std::size_t N>
        explicit constexpr ClassIDType(const char (&fourLetterCode)[N])
            : AsBytes {
                  fourLetterCode[0],
                  fourLetterCode[1],
                  fourLetterCode[2],
                  fourLetterCode[3] }
        {
            static_assert(N >= 4, "should be passing in a four-letter code.");
        }
    };

    IDType ID;
    ClassIDType ClassID;
    std::shared_ptr<void> Data;

    ResourceHandle()
        : ID(0)
        , ClassID(0)
    { }

    template<class T>
    ResourceHandle(IDType id,
                   ClassIDType classID,
                   std::shared_ptr<T> data)
        : ID(id)
        , ClassID(classID)
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
