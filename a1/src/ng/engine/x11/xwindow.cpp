#include "ng/engine/x11/xwindow.hpp"

#include "ng/engine/window.hpp"

#include "ng/engine/x11/xglcontext.hpp"

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

class ngXDisplay
{
public:
    Display* mHandle;

    ngXDisplay()
    {
        mHandle = XOpenDisplay(NULL);
        if (!mHandle)
        {
            throw std::runtime_error("Cannot connect to X server");
        }
    }

    ~ngXDisplay()
    {
        if (mHandle)
        {
            XCloseDisplay(mHandle);
        }
    }

    ngXDisplay(ngXDisplay&& other)
        : mHandle(nullptr)
    {
        swap(other);
    }

    ngXDisplay& operator=(ngXDisplay&& other)
    {
        swap(other);
    }

    void swap(ngXDisplay& other)
    {
        using std::swap;
        swap(mHandle, other.mHandle);
    }
};

void swap(ngXDisplay& a, ngXDisplay& b)
{
    a.swap(b);
}

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
        int width, int height,
        int x, int y,
        const WindowFlags& flags)
{
    // arbitrarily put here to force the linker to realize pthreads is necessary... driver bug. (see pthreadhack.hpp)
    ForcePosixThreadsLink();

    const int attribList[] =
    {
        GLX_X_RENDERABLE    , True,
        GLX_DRAWABLE_TYPE   , GLX_WINDOW_BIT,
        GLX_RENDER_TYPE     , GLX_RGBA_BIT,
        GLX_X_VISUAL_TYPE   , GLX_TRUE_COLOR,
        GLX_RED_SIZE        , flags.RedSize,
        GLX_GREEN_SIZE      , flags.GreenSize,
        GLX_BLUE_SIZE       , flags.BlueSize,
        GLX_ALPHA_SIZE      , flags.AlphaSize,
        GLX_DEPTH_SIZE      , flags.DepthSize,
        GLX_STENCIL_SIZE    , flags.StencilSize,
        GLX_DOUBLEBUFFER    , flags.DoubleBuffered ? True : False,
        //GLX_SAMPLE_BUFFERS  , 1,
        //GLX_SAMPLES         , 4,
        None
    };

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

    XSetWindowAttributes setWindowAttributes;
    setWindowAttributes.colormap = colormap.mHandle;
    setWindowAttributes.background_pixmap = None;
    setWindowAttributes.border_pixel = 0;
    setWindowAttributes.event_mask = StructureNotifyMask;

    Window root = DefaultRootWindow(display);
    return std::unique_ptr<IWindow>(
                new ngXWindow(
                    std::move(ngDisplay),
                    std::move(colormap),
                    root,
                    &setWindowAttributes,
                    CWColormap | CWBackPixmap | CWBorderPixel | CWEventMask,
                    title,
                    x, y,
                    width, height,
                    vi->depth,
                    vi->visual,
                    attribList,
                    bestFBC));
}

} // end namespace ng
