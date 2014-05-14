#include "ng/engine/x11/xvisualinfo.hpp"

#include <GL/glx.h>

#include <stdexcept>

namespace ng
{

ngXVisualInfo::ngXVisualInfo(Display* dpy, int screen, const int* attribList)
{
    mHandle = glXChooseVisual(dpy, screen, (int*) attribList);
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
