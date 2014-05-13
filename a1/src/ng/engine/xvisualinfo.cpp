#include "ng/engine/xvisualinfo.hpp"

namespace ng
{

ngXVisualInfo::ngXVisualInfo(Display* dpy, int screen, const int* attribList)
{
    mHandle = glXChooseVisual(display, screen, attribList);
    if (!mHandle)
    {
        throw std::runtime_error("No appropriate visual found");
    }
}

ngXVisualInfo::~ngXVisualInfo()
{
    XFree(mHandle);
}

} // end namespace ng
