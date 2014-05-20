#include "ng/engine/windowmanager.hpp"

#ifdef NG_USE_X11
#include "ng/engine/x11/xwindow.hpp"
#endif

namespace ng
{

std::shared_ptr<IWindowManager> CreateWindowManager()
{
#ifdef NG_USE_X11
    return CreateXWindowManager();
#else
    static_assert(false, "No implementation of CreateWindowManager for this configuration.");
#endif
}

} // end namespace ng
