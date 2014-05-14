#ifndef NG_XVISUALINFO_HPP
#define NG_XVISUALINFO_HPP

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

namespace ng
{

class ngXVisualInfo
{
public:
    XVisualInfo* mHandle;

    ngXVisualInfo(Display* dpy, int screen, const int* attribList);
    ~ngXVisualInfo();
};

} // end namespace ng

#endif // NG_XVISUALINFO_HPP
