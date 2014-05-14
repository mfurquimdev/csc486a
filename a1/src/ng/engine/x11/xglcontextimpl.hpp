#ifndef NG_XGLCONTEXTIMPL_HPP
#define NG_XGLCONTEXTIMPL_HPP

#include "ng/engine/x11/xdisplay.hpp"
#include "ng/engine/x11/xvisualinfo.hpp"

#include "ng/engine/iglcontext.hpp"

#include <GL/glx.h>

namespace ng
{

class ngXGLContextImpl
{
public:
    Display* mDisplay;
    GLXContext mHandle;

    ngXGLContextImpl(Display* dpy, XVisualInfo* vi, GLXContext shareList, Bool direct);

    ~ngXGLContextImpl();
};

class ngXGLContext : public IGLContext
{
public:
    ngXDisplay mDisplay;
    ngXVisualInfo mVisualInfo;
    ngXGLContextImpl mContext;

    ngXGLContext(const int* attribList);
};

} // end namespace ng

#endif // NG_XGLCONTEXTIMPL_HPP
