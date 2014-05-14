#include "ng/engine/x11/xglcontextimpl.hpp"

#include <cassert>

namespace ng
{

ngXGLContextImpl::ngXGLContextImpl(Display* dpy, XVisualInfo* vi, GLXContext shareList, Bool direct)
    : mDisplay(dpy)
{
    // rely on XErrorHandler to throw if this fails
    mHandle = glXCreateContext(dpy, vi, shareList, direct);
    assert(mHandle);
}

ngXGLContextImpl::~ngXGLContextImpl()
{
    glXDestroyContext(mDisplay, mHandle);
}

ngXGLContext::ngXGLContext(const int* attribList)
    : mVisualInfo(mDisplay.mHandle, 0, attribList)
    , mContext(mDisplay.mHandle, mVisualInfo.mHandle, 0, GL_TRUE)
{ }

} // end namespace ng
