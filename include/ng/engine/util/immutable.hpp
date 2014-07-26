#ifndef NG_IMMUTABLE_HPP
#define NG_IMMUTABLE_HPP

#include <memory>

namespace ng
{

template<class T>
class immutable : public std::enable_shared_from_this<immutable<T>>
{
    T mValue;

public:
    explicit immutable(T value)
        : mValue(std::move(value))
    { }

    const T& get() const
    {
        return mValue;
    }

    // only allowed if this is the sole reference
    // and if this is wrapped in a shared_ptr
    // otherwise, UB.
    T& get_mutable()
    {
        if (shared_from_this().unique())
        {
            return mValue;
        }
        else
        {
            throw std::logic_error(
                        "Tried getting mutable access to "
                        "non-uniquely owned object");
        }
    }
};

} // end namespace ng

#endif // NG_IMMUTABLE_HPP
