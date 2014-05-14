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

    ngXDisplay(ngXDisplay&& other);
    ngXDisplay& operator=(ngXDisplay&& other);
    void swap(ngXDisplay& other);
};

void swap(ngXDisplay& a, ngXDisplay& b);

} // end namespace ng

#endif // NG_XDISPLAY_HPP
