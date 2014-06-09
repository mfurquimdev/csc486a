#ifndef NG_SCOPEGUARD_HPP
#define NG_SCOPEGUARD_HPP

#include <type_traits>
#include <utility>

namespace ng
{

template<class F>
class scope_guard
{
    typename std::remove_reference<F>::type mF;

public:
    scope_guard(F&& f)
        : mF(std::forward<F>(f))
    { }

    ~scope_guard()
    {
        mF();
    }
};

template<class F>
scope_guard<F> make_scope_guard(F&& f)
{
    return { std::forward<F>(f) };
}

} // end namespace ng

#endif // NG_SCOPEGUARD_HPP
