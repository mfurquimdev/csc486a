#ifndef NG_XERRORHANDLER_HPP
#define NG_XERRORHANDLER_HPP

#include <X11/Xlib.h>

namespace ng
{

int ngXErrorHandler(Display* dpy, XErrorEvent* error);

struct ScopedErrorHandler
{
    using ErrorHandler = int(*)(Display*, XErrorEvent*);

    ErrorHandler mOldHandler;

    ScopedErrorHandler(ErrorHandler newHandler);
    ~ScopedErrorHandler();
};

} // end namespace ng

#endif // NG_XERRORHANDLER_HPP
