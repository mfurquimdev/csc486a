#include "ng/engine/x11/xdisplay.hpp"

#include <stdexcept>

namespace ng
{

ngXDisplay::ngXDisplay()
{
    mHandle = XOpenDisplay(NULL);
    if (!mHandle)
    {
        throw std::runtime_error("Cannot connect to X server");
    }
}

ngXDisplay::~ngXDisplay()
{
    XCloseDisplay(mHandle);
}

} // end namespace ng
