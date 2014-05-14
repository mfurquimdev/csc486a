#include "ng/engine/x11/xwindow.hpp"

#include "ng/engine/iwindow.hpp"

#include "ng/engine/x11/xglcontextimpl.hpp"

#include "ng/engine/x11/xdisplay.hpp"
#include "ng/engine/x11/xvisualinfo.hpp"
#include "ng/engine/x11/xerrorhandler.hpp"

#include "ng/engine/x11/pthreadhack.hpp"

#include <X11/X.h>
#include <X11/Xlib.h>
#include <GL/glx.h>

#include <stdexcept>

namespace ng
{

class ngXColormap
{
public:
    Display* mDisplay;
    Colormap mHandle;

    ngXColormap(Display* display, Window w, Visual* visual, int alloc)
        : mDisplay(display)
        // rely on XErrorHandler to throw if this fails
        , mHandle(XCreateColormap(display, w, visual, alloc))
    { }

    ~ngXColormap()
    {
        XFreeColormap(mDisplay, mHandle);
    }
};

class ngXSetWindowAttributes
{
public:
    XSetWindowAttributes mAttributes;
    unsigned long mAttributeMask;

    ngXSetWindowAttributes()
        : mAttributeMask(0)
    { }

    ngXSetWindowAttributes& SetColormap(Colormap colormap)
    {
        mAttributes.colormap = colormap;
        mAttributeMask |= CWColormap;
        return *this;
    }

    ngXSetWindowAttributes& SetEventMask(long event_mask)
    {
        mAttributes.event_mask = event_mask;
        mAttributeMask |= CWEventMask;
        return *this;
    }
};

class ngXWindowImpl
{
public:
    Display* mDisplay;
    Window mHandle;

    ngXWindowImpl(const char* title,
              Display* display, Window parent,
              int x, int y,
              unsigned int width, unsigned int height,
              unsigned int border_width,
              int depth,
              unsigned int clazz,
              Visual* visual,
              unsigned long valuemask,
              XSetWindowAttributes* attributes)
        : mDisplay(display)
        // rely on XErrorHandler to throw if this fails
        , mHandle(XCreateWindow(display, parent, x, y, width, height, border_width, depth, clazz, visual, valuemask, attributes))
    {
        XMapWindow(mDisplay, mHandle);
        XStoreName(mDisplay, mHandle, title);
    }

    ~ngXWindowImpl()
    {
        XDestroyWindow(mDisplay, mHandle);
    }
};

class ngXWindow : public IWindow
{
public:
    ngXDisplay mDisplay;
    Window mRoot;
    ngXVisualInfo mVisualInfo;
    ngXColormap mColorMap;
    ngXSetWindowAttributes mSetWindowAttributes;
    ngXWindowImpl mWindow;

    ngXWindow(const char*  title, int x, int y, int width, int height, const int* attribList)
        : mRoot(DefaultRootWindow(mDisplay.mHandle))
        , mVisualInfo(mDisplay.mHandle, 0, attribList)
        , mColorMap(mDisplay.mHandle, mRoot, mVisualInfo.mHandle->visual, None)
        , mSetWindowAttributes(ngXSetWindowAttributes()
                               .SetColormap(mColorMap.mHandle)
                               .SetEventMask(ExposureMask | KeyPressMask))
        , mWindow(title,
                  mDisplay.mHandle, mRoot,
                  x, y,
                  width, height,
                  0,
                  mVisualInfo.mHandle->depth,
                  InputOutput,
                  mVisualInfo.mHandle->visual,
                  mSetWindowAttributes.mAttributeMask, &mSetWindowAttributes.mAttributes)
    { }

    void SwapBuffers() override
    {
        glXSwapBuffers(mDisplay.mHandle, mWindow.mHandle);
    }

    void GetSize(int* width, int* height) override
    {
        XWindowAttributes attributes;
        XGetWindowAttributes(mDisplay.mHandle, mWindow.mHandle, &attributes);

        if (width) *width = attributes.width;
        if (height) *height = attributes.height;
    }

    void MakeCurrent(const IGLContext& context) override
    {
        const ngXGLContext& xcontext = static_cast<const ngXGLContext&>(context);
        glXMakeCurrent(mDisplay.mHandle, mWindow.mHandle, xcontext.mContext.mHandle);
    }
};

std::unique_ptr<IWindow> CreateXWindow(const char* title, int x, int y, int width, int height, const int* attribList)
{
    // arbitrarily put here to force the linker to realize pthreads is necessary... driver bug. (see pthreadhack.hpp)
    ForcePosixThreadsLink();

    XSetErrorHandler(ngXErrorHandler);
    return std::unique_ptr<IWindow>(new ngXWindow(title, x, y, width, height, attribList));
}

} // end namespace ng
