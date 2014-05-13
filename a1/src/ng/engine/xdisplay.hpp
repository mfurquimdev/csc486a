#ifndef NG_XDISPLAY_HPP
#define NG_XDISPLAY_HPP

#include <X11/Xlib.h>

namespace ng
{

class ngXDisplay
{
public:
    Display* mHandle;

    ngXDisplay();
    ~ngXDisplay();
};

} // end namespace ng

#endif // NG_XDISPLAY_HPP
