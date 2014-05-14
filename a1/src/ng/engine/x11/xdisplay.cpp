#include "ng/engine/x11/xdisplay.hpp"

#include <stdexcept>
#include <utility>

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
    if (mHandle)
    {
        XCloseDisplay(mHandle);
    }
}

ngXDisplay::ngXDisplay(ngXDisplay&& other)
    : mHandle(nullptr)
{
    swap(other);
}

ngXDisplay& ngXDisplay::operator=(ngXDisplay&& other)
{
    swap(other);
}

void ngXDisplay::swap(ngXDisplay& other)
{
    using std::swap;
    swap(mHandle, other.mHandle);
}

void swap(ngXDisplay& a, ngXDisplay& b)
{
    a.swap(b);
}

} // end namespace ng
