#include "ng/engine/x11/xwindow.hpp"

#include "ng/engine/iwindow.hpp"

#include "ng/engine/x11/xglcontext.hpp"

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
        if (mHandle)
        {
            XFreeColormap(mDisplay, mHandle);
        }
    }

    ngXColormap(ngXColormap&& other)
        : mDisplay(nullptr)
        , mHandle(0)
    {
        swap(other);
    }

    ngXColormap& operator=(ngXColormap&& other)
    {
        swap(other);
    }

    void swap(ngXColormap& other)
    {
        using std::swap;
        swap(mDisplay, other.mDisplay);
        swap(mHandle, other.mHandle);
    }
};

void swap(ngXColormap& a, ngXColormap& b)
{
    a.swap(b);
}

class ngXSetWindowAttributes
{
public:
    XSetWindowAttributes mAttributes;
    unsigned long mAttributeMask;

    ngXSetWindowAttributes()
        : mAttributeMask(0)
    { }

    void SetColormap(Colormap colormap)
    {
        mAttributes.colormap = colormap;
        mAttributeMask |= CWColormap;
    }

    void SetBackgroundPixmap(Pixmap pixmap)
    {
        mAttributes.background_pixmap = pixmap;
        mAttributeMask |= CWBackPixmap;
    }

    void SetBorderPixel(unsigned long border_pixel)
    {
        mAttributes.border_pixel = border_pixel;
        mAttributeMask |= CWBorderPixel;
    }

    void SetEventMask(long event_mask)
    {
        mAttributes.event_mask = event_mask;
        mAttributeMask |= CWEventMask;
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
    ngXColormap mColormap;
    ngXWindowImpl mWindow;
    GLXFBConfig mChosenFBC;

    ngXWindow(ngXDisplay&& display,
              ngXColormap&& colormap,
              Window parent,
              const XSetWindowAttributes* setAttributes,
              unsigned long setAttributeMask,
              const char*  title,
              int x, int y,
              int width, int height,
              int depth,
              Visual* visual,
              const int* attribList,
              GLXFBConfig chosenFBC)
        : mDisplay(std::move(display))
        , mColormap(std::move(colormap))
        , mWindow(title,
                  mDisplay.mHandle, parent,
                  x, y,
                  width, height,
                  0,
                  depth,
                  InputOutput,
                  visual,
                  setAttributeMask, (XSetWindowAttributes*) setAttributes)
        , mChosenFBC(chosenFBC)
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

    std::unique_ptr<IGLContext> CreateContext() override
    {
        return std::unique_ptr<IGLContext>(new ngXGLContext(mDisplay.mHandle, mChosenFBC));
    }

    void MakeCurrent(const IGLContext& context) override
    {
        const ngXGLContext& xcontext = static_cast<const ngXGLContext&>(context);
        glXMakeCurrent(mDisplay.mHandle, mWindow.mHandle, xcontext.mHandle);
    }
};

std::unique_ptr<IWindow> CreateXWindow(
        const char* title,
        int x, int y,
        int width, int height,
        const int* attribList)
{
    // arbitrarily put here to force the linker to realize pthreads is necessary... driver bug. (see pthreadhack.hpp)
    ForcePosixThreadsLink();

    ScopedErrorHandler errorHandler(ngXErrorHandler);

    ngXDisplay ngDisplay{};
    Display* display = ngDisplay.mHandle;

    // Check glx version
    int glxMajorVersion = 0, glxMinorVersion = 0;
    if (!glXQueryVersion(display, &glxMajorVersion, &glxMinorVersion)
            || glxMajorVersion < 1 || (glxMajorVersion == 1 && glxMinorVersion < 3))
    {
        std::string msg;

        if (glxMajorVersion == 0 && glxMinorVersion == 0)
        {
            msg = "Failed to query version with glxQueryVersion";
        }
        else
        {
            msg = "Invalid GLX Version: "
                    + std::to_string(glxMajorVersion) + "." + std::to_string(glxMinorVersion)
                    + " (need GLX version 1.3 for FBConfigs";
        }

        throw std::runtime_error(msg);
    }

    // Get framebuffer configs
    int fbCount;
    std::unique_ptr<GLXFBConfig, int(*)(void*)> fbConfigList(
                glXChooseFBConfig(display, DefaultScreen(display), attribList, &fbCount),
                XFree);
    if (!fbConfigList)
    {
        throw std::runtime_error("Failed to retrieve a framebuffer config");
    }
    GLXFBConfig* fbc = fbConfigList.get();

    // Pick the FB config/visual with the most samples per pixel
    int bestFBCIndex = -1, bestNumSamples = -1;
    for (int i = 0; i < fbCount; i++)
    {
        std::unique_ptr<XVisualInfo, int(*)(void*)> vi(
                    glXGetVisualFromFBConfig(display, fbc[i]),
                    XFree);
        if (vi)
        {
            int sampleBuffers, samples;
            int sampleBufferSupport = glXGetFBConfigAttrib(display, fbc[i], GLX_SAMPLE_BUFFERS, &sampleBuffers);
            int samplesSupport = glXGetFBConfigAttrib(display, fbc[i], GLX_SAMPLES, &samples);
            if (sampleBufferSupport == GLX_NO_EXTENSION || sampleBufferSupport == GLX_BAD_ATTRIBUTE
                || samplesSupport == GLX_NO_EXTENSION || samplesSupport == GLX_BAD_ATTRIBUTE)
            {
                continue;
            }

            if (bestFBCIndex < 0 || sampleBuffers && samples > bestNumSamples)
            {
                bestFBCIndex = i;
                bestNumSamples = samples;
            }
        }
    }

    GLXFBConfig bestFBC = fbc[bestFBCIndex];

    // Get a visual
    std::unique_ptr<XVisualInfo, int(*)(void*)> vi(
                glXGetVisualFromFBConfig(display, bestFBC),
                XFree);

    // Create color map
    ngXColormap colormap(display, RootWindow(display, vi->screen), vi->visual, AllocNone);

    ngXSetWindowAttributes setWindowAttributes;
    setWindowAttributes.SetColormap(colormap.mHandle);
    setWindowAttributes.SetBackgroundPixmap(None);
    setWindowAttributes.SetBorderPixel(0);
    setWindowAttributes.SetEventMask(StructureNotifyMask);

    Window root = DefaultRootWindow(display);
    return std::unique_ptr<IWindow>(
                new ngXWindow(
                    std::move(ngDisplay),
                    std::move(colormap),
                    root,
                    &setWindowAttributes.mAttributes,
                    setWindowAttributes.mAttributeMask,
                    title,
                    x, y,
                    width, height,
                    vi->depth,
                    vi->visual,
                    attribList,
                    bestFBC));
}

} // end namespace ng
