#include "ng/engine/xwindow.hpp"

#include "ng/engine/xdisplay.hpp"
#include "ng/engine/xvisualinfo.hpp"

#include <X11/X.h>
#include <X11/Xlib.h>
#include <GL/glx.h>

#include <stdexcept>

namespace ng
{

static int ngXErrorHandler(Display* dpy, XErrorEvent* error)
{
    char errorBuf[256];
    errorBuf[0] = '\0';
    XGetErrorText(dpy, error->error_code, errorBuf, sizeof(errorBuf));
    throw std::runtime_error(errorBuf);
}

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

class ngXWindow
{
public:
    Display mDisplay;
    Window mHandle;

    ngXWindow(const char* title,
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

    ~ngXWindow()
    {
        XDestroyWindow(mDisplay, mHandle);
    }
};

class ngXWindowContainer : public IWindow
{
public:
    ngXDisplay mDisplay;
    Window mRoot;
    ngXVisualInfo mVisualInfo;
    ngXColormap mColorMap;
    ngXSetWindowAttributes mSetWindowAttributes;
    ngXWindow mWindow;

    ngXWindowContainer(const char*  title, int x, int y, int width, int height, const int* attribList)
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
};

std::unique_ptr<IWindow> CreateXWindow(const char* title, int x, int y, int width, int height, const int* attribList)
{
    XSetErrorHandler(ngXErrorHandler);
    return new ngXWindowContainer(title, x, y, width, height, attribList);
}

} // end namespace ng
