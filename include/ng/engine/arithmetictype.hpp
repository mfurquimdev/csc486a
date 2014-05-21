#ifndef NG_ARITHMETICTYPE_HPP
#define NG_ARITHMETICTYPE_HPP

#include <cstdint>
#include <stdexcept>

namespace ng
{

enum class ArithmeticType
{
    Int8,
    Int16,
    Int32,
    Int64,
    UInt8,
    UInt16,
    UInt32,
    UInt64,
    Float,
    Double
};

constexpr std::size_t SizeOfArithmeticType(ArithmeticType at)
{
    return at == ArithmeticType::Int8 ? sizeof(std::int8_t)
         : at == ArithmeticType::Int16 ? sizeof(std::int16_t)
         : at == ArithmeticType::Int32 ? sizeof(std::int32_t)
         : at == ArithmeticType::Int64 ? sizeof(std::int64_t)
         : at == ArithmeticType::UInt8 ? sizeof(std::uint8_t)
         : at == ArithmeticType::UInt16 ? sizeof(std::uint16_t)
         : at == ArithmeticType::UInt32 ? sizeof(std::uint32_t)
         : at == ArithmeticType::UInt64 ? sizeof(std::uint64_t)
         : at == ArithmeticType::Float ? sizeof(float)
         : at == ArithmeticType::Double ? sizeof(double)
         : throw std::logic_error("No such ArithmeticType");
}

} // end namespace ng

#endif // NG_ARITHMETICTYPE_HPP
