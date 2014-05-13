#include "ng/engine/xglcontext.hpp"

#include "ng/engine/xdisplay.hpp"
#include "ng/engine/xvisualinfo.hpp"

#include <GL/glx.h>

namespace ng
{

class ngXGLContext
{
public:
    Display mDisplay;
    GLXContext mHandle;

    ngXGLContext(Display* dpy, XVisualInfo* vi, GLXContext shareList, Bool direct)
        : mDisplay(dpy)
    {
        // rely on XErrorHandler to throw if this fails
        mHandle = glXCreateContext(dpy, vi, shareList, direct);
    }

    ~ngXGLContext()
    {
        glXDestroyContext(mDisplay, mHandle);
    }
};

class ngXGLContextContainer : public IGLContext
{
public:
    ngXDisplay mDisplay;
    ngXVisualInfo mVisualInfo;
    ngXGLContext mContext;

    ngXGLContextContainer(const int* attribList)
        : mVisualInfo(mDisplay.mHandle, 0, attribList)
        , mContext(mDisplay.mHandle, mVisualInfo.mHandle, 0, GL_TRUE)
    { }
};

std::unique_ptr<IGLContext> CreateXGLContext(const int* attribList)
{
    return new ngXGLContext(attribList);
}

} // end namespace ng
