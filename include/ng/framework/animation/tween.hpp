#ifndef NG_TWEEN_HPP
#define NG_TWEEN_HPP

#include <cmath>

namespace ng
{

namespace tween
{

// t: current time
// b: start value
// c: total distance to travel (final value - start value)
// d: duration

template<class T>
T Linear(T t, T b, T c, T d)
{
    return c * t / d + b;
}

template<class T>
T EaseInQuad(T t, T b, T c, T d)
{
    t /= d;
    return c * t * t + b;
}

template<class T>
T EaseOutQuad(T t, T b, T c, T d)
{
    t /= d;
    return - c * t * (t - T(2)) + b;
}

template<class T>
T EaseInOutQuad(T t, T b, T c, T d)
{
    t /= d / T(2);
    if (t < T(1)) return c / T(2) * t * t + b;
    t -= T(1);
    return - c / T(2) * (t * (t - T(2)) - T(1)) + b;
}

template<class T>
T EaseInCubic(T t, T b, T c, T d)
{
    t /= d;
    return c * t * t * t + b;
}

template<class T>
T EaseOutCubic(T t, T b, T c, T d)
{
    t /= d;
    t -= T(1);
    return c * (t * t * t + T(1)) + b;
}

template<class T>
T EaseInOutCubic(T t, T b, T c, T d)
{
    t /= d / T(2);
    if (t < T(1)) return c / T(2) * t * t * t + b;
    t -= T(2);
    return c / T(2) * (t * t * t + T(2)) + b;
}

template<class T>
T EaseInQuart(T t, T b, T c, T d)
{
    t /= d;
    return c * t * t * t * t + b;
}

template<class T>
T EaseOutQuart(T t, T b, T c, T d)
{
    t /= d;
    t -= T(1);
    return - c * (t * t * t * t - T(1)) + b;
}

template<class T>
T EaseInOutQuart(T t, T b, T c, T d)
{
    t /= d / T(2);
    if (t < T(1)) return c / T(2) * t * t * t * t + b;
    t -= T(2);
    return - c / T(2) * (t * t * t * t - T(2)) + b;
}

template<class T>
T EaseInQuint(T t, T b, T c, T d)
{
    t /= d;
    return c * t * t * t * t * t + b;
}

template<class T>
T EaseOutQuint(T t, T b, T c, T d)
{
    t /= d;
    t -= T(1);
    return c * (t * t * t * t * t + T(1)) + b;
}

template<class T>
T EaseInOutQuint(T t, T b, T c, T d)
{
    t /= d / T(2);
    if (t < T(1)) return c / T(2) * t * t * t * t * t + b;
    t -= T(2);
    return c / T(2) * (t * t * t * t * t + T(2)) + b;
}

template<class T>
T EaseInSine(T t, T b, T c, T d)
{
    using std::cos;

    return - c * cos(t / d * pi<T>::value / T(2)) + c + b;
}

template<class T>
T EaseOutSine(T t, T b, T c, T d)
{
    using std::sin;

    return c * sin(t / d * pi<T>::value / T(2)) + b;
}

template<class T>
T EaseInOutSine(T t, T b, T c, T d)
{
    using std::cos;

    return - c / T(2) * (cos(pi<T>::value * t / d) - T(1)) + b;
}

template<class T>
T EaseInExpo(T t, T b, T c, T d)
{
    using std::pow;

    return c * pow(T(2), T(10) * (t / d - T(1))) + b;
}

template<class T>
T EaseOutExpo(T t, T b, T c, T d)
{
    using std::pow;

    return c * (- pow(T(2), T(-10) * t / d) + T(1)) + b;
}

template<class T>
T EaseInOutExpo(T t, T b, T c, T d)
{
    using std::pow;

    t /= d / T(2);
    if (t < T(1)) return c / T(2) * pow(T(2), T(10) * (t - T(1))) + b;
    t -= T(1);
    return c / T(2) * (-pow(T(2), T(-10) * t) + T(2)) + b;
}

template<class T>
T EaseInCirc(T t, T b, T c, T d)
{
    using std::sqrt;

    t /= d;
    return - c * (sqrt(T(1) - t * t) - T(1)) + b;
}

template<class T>
T EaseOutCirc(T t, T b, T c, T d)
{
    using std::sqrt;

    t /= d;
    t -= T(1);
    return c * sqrt(T(1) - t * t) + b;
}

template<class T>
T EaseInOutCirc(T t, T b, T c, T d)
{
    using std::sqrt;

    t /= d / T(2);
    if (t < T(1)) return - c / T(2) * (sqrt(T(1) - t * t) - T(1)) + b;
    t -= T(2);
    return c / T(2) * (sqrt(T(1) - t * t) + T(1)) + b;
}

} // end namespace tween

} // end namespace ng

#endif // NG_TWEEN_HPP
