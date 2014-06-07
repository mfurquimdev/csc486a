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
    virtual ~IApp() = default;

    virtual void Init() = 0;
    virtual AppStepAction Step() = 0;
};

// define this in your application to provide the engine with an entry point.
extern std::shared_ptr<IApp> CreateApp();

} // end namespace ng

#endif // NG_APP_HPP
