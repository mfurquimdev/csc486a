#ifndef NG_APP_HPP
#define NG_APP_HPP

#include <memory>

namespace ng
{

enum class AppStepAction
{
    Continue,
    Quit
};

class IApp
{
public:
    virtual void Init() = 0;
    virtual AppStepAction Step() = 0;
};

extern std::unique_ptr<IApp> CreateApp();

} // end namespace ng

#endif // NG_APP_HPP
