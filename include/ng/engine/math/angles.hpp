#ifndef NG_ANGLES_HPP
#define NG_ANGLES_HPP

#include "ng/engine/math/constants.hpp"

namespace ng
{

template<class T>
class Radians
{
public:
    T Value;

    explicit Radians(T rads)
        : Value(rads)
    { }

    explicit operator T() const
    {
        return Value;
    }
};

using Radiansf = Radians<float>;
using Radiansd = Radians<double>;

template<class T>
class Degrees
{
public:
    T Value;

    explicit Degrees(T degs)
        : Value(degs)
    { }

    explicit Degrees(Radians<T> rads)
        : Value(rads.Value * 180 / pi<T>::value)
    { }

    explicit operator Radians<T>()
    {
        return Radians<T>(Value * pi<T>::value / 180);
    }

    explicit operator T() const
    {
        return Value;
    }
};

using Degreesf = Degrees<float>;
using Degreesd = Degrees<double>;

} // end namespace ng

#endif // NG_ANGLES_HPP
