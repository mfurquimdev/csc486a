#include "ng/engine/x11/xwindow.hpp"

#include "ng/engine/window.hpp"
#include "ng/engine/glcontext.hpp"
#include "ng/engine/windowmanager.hpp"
#include "ng/engine/windowevent.hpp"

#include <X11/X.h>
#include <X11/Xlib.h>
#include <GL/glx.h>

#include <vector>
#include <stdexcept>
#include <algorithm>
#include <cstring>
#include <mutex>

namespace ng
{

static std::mutex gX11Lock;

int ngXErrorHandler(Display* dpy, XErrorEvent* error)
{
    std::array<char,256> errorTextBuf;
    errorTextBuf[0] = '\0';
    XGetErrorText(dpy, error->error_code, errorTextBuf.data(), errorTextBuf.size());

    std::string msg = "Error " + std::to_string(error->error_code)
            + " (" + errorTextBuf.data() + "): request "
            + std::to_string(error->request_code) + "." + std::to_string(error->minor_code);

    throw std::runtime_error(msg);
}

struct ScopedErrorHandler
{
    using ErrorHandler = int(*)(Display*, XErrorEvent*);

    ErrorHandler mOldHandler;

    ScopedErrorHandler(ErrorHandler newHandler)
    {
        mOldHandler = XSetErrorHandler(newHandler);
    }

    ~ScopedErrorHandler()
    {
        XSetErrorHandler(mOldHandler);
    }
};


template<class T>
static void ngXLockedDelete(T* ptr)
{
    std::lock_guard<std::mutex> scopedX11Lock(gX11Lock);
    ScopedErrorHandler errorHandler(ngXErrorHandler);
    delete ptr;
}

class ngXGLContext : public IGLContext
{
public:
    Display* mDisplay;
    GLXContext mHandle;

    ngXGLContext(Display* dpy, GLXFBConfig config, GLXContext shareList)
        : mDisplay(dpy)
    {
        using glXCreateContextAttribsARBProc = GLXContext(*)(Display*, GLXFBConfig, GLXContext, Bool, const int*);

        glXCreateContextAttribsARBProc glXCreateContextAttribsARB = 0;
        glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc) GetProcAddress_NoLock("glXCreateContextAttribsARB");

        if (!IsExtensionSupported_NoLock("GLX_ARB_create_context") ||
            !glXCreateContextAttribsARB)
        {
            // glXCreateContextAttribsARB() not found
            // use old style GLX context
            mHandle = glXCreateNewContext(dpy, config, GLX_RGBA_TYPE, shareList, True);
        }
        else
        {
            // ouch
            static bool sFailedToBuildContext;
            static XErrorEvent sLastError;
            sFailedToBuildContext = false;

            struct RepushLastError
            {
                ~RepushLastError()
                {
                    if (sFailedToBuildContext)
                    {
                        XEvent e;
                        e.xerror = sLastError;
                        XPutBackEvent(sLastError.display, &e);
                        XSync(sLastError.display, False);
                    }
                }
            };

            ScopedErrorHandler errorHandler([](Display*, XErrorEvent* error)
            {
                sFailedToBuildContext = true;
                sLastError = *error;
                return 0;
            });

            // if glXCreateContextAttribsARB is found, try to get a 3.0 context.
            int contextAttribs[] = {
                GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
                GLX_CONTEXT_MINOR_VERSION_ARB, 0,
                None
            };

            mHandle = glXCreateContextAttribsARB(dpy, config, shareList, True, contextAttribs);

            // flush errors
            XSync(dpy, False);
            if (sFailedToBuildContext || !mHandle)
            {
                // Couldn't create a GL 3.0 context, so fall back to 2.x context.
                contextAttribs[1] = 1;
                contextAttribs[3] = 0;

                mHandle = glXCreateContextAttribsARB(dpy, config, shareList, True, contextAttribs);
                XSync(dpy, False);
            }
        }

        if (!mHandle)
        {
            // hopefully X11's error reporting catches errors before this does.
            throw std::runtime_error("Failed to build context");
        }
    }

    ~ngXGLContext()
    {
        glXDestroyContext(mDisplay, mHandle);
    }

    bool IsExtensionSupported_NoLock(const char* extension)
    {
        const char *extList = glXQueryExtensionsString(mDisplay, DefaultScreen(mDisplay));

        const char *start;
        const char *where, *terminator;

        /* Extension names should not have spaces. */
        where = std::strchr(extension, ' ');
        if (where || *extension == '\0')
            return false;

        size_t extlen = std::strlen(extension);
        /* It takes a bit of care to be fool-proof about parsing the
             OpenGL extensions string. Don't be fooled by sub-strings,
             etc. */
        for (start=extList;;) {
            where = std::strstr(start, extension);

            if (!where)
                break;

            terminator = where + extlen;

            if ( where == start || *(where - 1) == ' ' )
                if ( *terminator == ' ' || *terminator == '\0' )
                    return true;

            start = terminator;
        }

        return false;
    }

    bool IsExtensionSupported(const char* extension) override
    {
        std::lock_guard<std::mutex> scopedX11Lock(gX11Lock);
        ScopedErrorHandler errorHandler(ngXErrorHandler);
        return IsExtensionSupported_NoLock(extension);
    }

    void* GetProcAddress_NoLock(const char *proc)
    {
        return (void*) glXGetProcAddressARB((const GLubyte*) proc);
    }

    void* GetProcAddress(const char *proc) override
    {
        std::lock_guard<std::mutex> scopedX11Lock(gX11Lock);
        ScopedErrorHandler errorHandler(ngXErrorHandler);
        return GetProcAddress_NoLock(proc);
    }
};

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
        return *this;
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
                  XSetWindowAttributes* attributes,
                  Atom* protocols,
                  int protocolCount)
        : mDisplay(display)
        // rely on XErrorHandler to throw if this fails
        , mHandle(XCreateWindow(display, parent, x, y, width, height, border_width, depth, clazz, visual, valuemask, attributes))
    {
        XSetWMProtocols(mDisplay, mHandle, protocols, protocolCount);
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
    VideoFlags mVideoFlags;
    Display* mDisplay;
    ngXColormap mColormap;
    ngXWindowImpl mWindow;
    GLXFBConfig mChosenFBC;

    ngXWindow(const VideoFlags& videoFlags,
              Display* display,
              ngXColormap&& colormap,
              Window parent,
              const XSetWindowAttributes* setAttributes,
              unsigned long setAttributeMask,
              const char*  title,
              int x, int y,
              int width, int height,
              int depth,
              Visual* visual,
              GLXFBConfig chosenFBC,
              Atom* protocols,
              int protocolCount)
        : mVideoFlags(videoFlags)
        , mDisplay(display)
        , mColormap(std::move(colormap))
        , mWindow(title,
                  mDisplay, parent,
                  x, y,
                  width, height,
                  0,
                  depth,
                  InputOutput,
                  visual,
                  setAttributeMask, (XSetWindowAttributes*) setAttributes,
                  protocols,
                  protocolCount)
        , mChosenFBC(chosenFBC)
    { }

    void SwapBuffers() override
    {
        std::lock_guard<std::mutex> scopedX11Lock(gX11Lock);
        ScopedErrorHandler scopedErrors(ngXErrorHandler);

        glXSwapBuffers(mDisplay, mWindow.mHandle);
    }

    void GetSize(int* width, int* height) override
    {
        std::lock_guard<std::mutex> scopedX11Lock(gX11Lock);
        ScopedErrorHandler scopedErrors(ngXErrorHandler);

        XWindowAttributes attributes;
        XGetWindowAttributes(mDisplay, mWindow.mHandle, &attributes);

        if (width) *width = attributes.width;
        if (height) *height = attributes.height;
    }

    const VideoFlags& GetVideoFlags() const override
    {
        return mVideoFlags;
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
        return *this;
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

class ngXWindowManager : public IWindowManager
{
    ngXDisplay mDisplay;
    Atom mWMDeleteMessage;

    struct WindowRecord
    {
        std::weak_ptr<ngXWindow> mWeakRef;
        Window mHandle;
    };

    std::vector<WindowRecord> mWindows;
    std::vector<std::weak_ptr<ngXGLContext>> mContexts;

public:
    ngXWindowManager()
        : mDisplay()
    {
        mWMDeleteMessage = XInternAtom(mDisplay.mHandle, "WM_DELETE_WINDOW", False);
    }

    static std::vector<int> VideoFlagsToAttribList(const VideoFlags& flags)
    {
        return
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
    }

    static GLXFBConfig GetBestFBConfig(Display* display, const int* attribList)
    {
        // Check glx version
        int glxMajorVersion = 0, glxMinorVersion = 0;
        if (!glXQueryVersion(display, &glxMajorVersion, &glxMinorVersion)
                || glxMajorVersion < 1
                || (glxMajorVersion == 1 && glxMinorVersion < 3))
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

                if (bestFBCIndex < 0 || (sampleBuffers && samples > bestNumSamples))
                {
                    bestFBCIndex = i;
                    bestNumSamples = samples;
                }
            }
        }

        return fbc[bestFBCIndex];
    }

    std::shared_ptr<IWindow> CreateWindow(const char* title,
                                          int width, int height,
                                          int x, int y,
                                          const VideoFlags& flags) override
    {
        // arbitrarily put here to force the linker to realize pthreads is necessary... driver bug.
        // see // https://bugs.launchpad.net/ubuntu/+source/nvidia-graphics-drivers-319/+bug/1248642
        pthread_getconcurrency();

        std::lock_guard<std::mutex> scopedX11Lock(gX11Lock);

        ScopedErrorHandler scopedErrors(ngXErrorHandler);

        std::vector<int> attribVector = VideoFlagsToAttribList(flags);
        int* attribList = attribVector.data();

        Display* display = mDisplay.mHandle;

        GLXFBConfig bestFBC = GetBestFBConfig(display, attribList);

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
        setWindowAttributes.event_mask = StructureNotifyMask | PointerMotionMask | ButtonPressMask | ButtonReleaseMask;

        Window root = DefaultRootWindow(display);
        std::shared_ptr<ngXWindow> createdWindow(
                    new ngXWindow(
                        flags,
                        display,
                        std::move(colormap),
                        root,
                        &setWindowAttributes,
                        CWColormap | CWBackPixmap | CWBorderPixel | CWEventMask,
                        title,
                        x, y, width, height,
                        vi->depth, vi->visual,
                        bestFBC,
                        &mWMDeleteMessage, 1),
                    ngXLockedDelete<ngXWindow>);

        // take this opportunity to GC the window list
        mWindows.erase(std::remove_if(mWindows.begin(), mWindows.end(),
                                      [](const WindowRecord& windowRecord)
        {
            return windowRecord.mWeakRef.expired();
        }), mWindows.end());

        mWindows.push_back({createdWindow, createdWindow->mWindow.mHandle});

        return createdWindow;
    }

    static bool MapXButton(unsigned int xbutton, MouseButton& button)
    {
        if (xbutton == Button1)
        {
            button = MouseButton::Left;
            return true;
        }
        else if (xbutton == Button2)
        {
            button = MouseButton::Middle;
            return true;
        }
        else if (xbutton == Button3)
        {
            button = MouseButton::Right;
            return true;
        }
        else
        {
            return false;
        }
    }

    bool PollEvent(WindowEvent& we) override
    {
        std::lock_guard<std::mutex> scopedX11Lock(gX11Lock);
        ScopedErrorHandler scopedErrors(ngXErrorHandler);

        while (XPending(mDisplay.mHandle) > 0)
        {
            XEvent ev;
            XNextEvent(mDisplay.mHandle, &ev);
            Window source = None;

            if (ev.type == ClientMessage &&
                (Atom) ev.xclient.data.l[0] == mWMDeleteMessage)
            {
                source = ev.xdestroywindow.window;
                we.type = WindowEventType::Quit;
            }
            else if (ev.type == MotionNotify)
            {
                source = ev.xmotion.window;
                we.motion.type = WindowEventType::MouseMotion;
                we.motion.x = ev.xmotion.x;
                we.motion.y = ev.xmotion.y;
            }
            else if (ev.type == ButtonPress)
            {
                if (!MapXButton(ev.xbutton.button, we.button.button))
                {
                    continue;
                }

                source = ev.xbutton.window;
                we.button.type = WindowEventType::MouseButton;
                we.button.state = ButtonState::Pressed;
                we.button.x = ev.xbutton.x;
                we.button.y = ev.xbutton.y;
            }
            else if (ev.type == ButtonRelease)
            {
                if (!MapXButton(ev.xbutton.button, we.button.button))
                {
                    continue;
                }

                source = ev.xbutton.window;
                we.button.type = WindowEventType::MouseButton;
                we.button.state = ButtonState::Released;
                we.button.x = ev.xbutton.x;
                we.button.y = ev.xbutton.y;
            }
            else
            {
                // unrecognized event type. move on to the next one.
                continue;
            }

            we.source.reset();

            if (source != None)
            {
                for (const auto& windowRecord : mWindows)
                {
                    if (windowRecord.mHandle == source)
                    {
                        we.source = windowRecord.mWeakRef;
                        break;
                    }
                }
            }

            return true;
        }

        return false;
    }

    std::shared_ptr<IGLContext> CreateContext(const VideoFlags& flags,
                                              std::shared_ptr<IGLContext> sharedWith) override
    {
        std::lock_guard<std::mutex> scopedX11Lock(gX11Lock);
        ScopedErrorHandler scopedErrors(ngXErrorHandler);

        std::vector<int> attribVector = VideoFlagsToAttribList(flags);

        GLXContext shareList = 0;
        if (sharedWith)
        {
            const ngXGLContext& toShareWith = static_cast<const ngXGLContext&>(*sharedWith);
            shareList = toShareWith.mHandle;
        }

        GLXFBConfig bestFBC = GetBestFBConfig(mDisplay.mHandle, attribVector.data());
        std::shared_ptr<ngXGLContext> context(new ngXGLContext(mDisplay.mHandle, bestFBC, shareList),
                                              ngXLockedDelete<ngXGLContext>);

        // take the opportunity to GC the contexts
        mContexts.erase(std::remove_if(mContexts.begin(), mContexts.end(),
                                      [](const std::weak_ptr<ngXGLContext>& pContext)
        {
            return pContext.expired();
        }), mContexts.end());

        mContexts.push_back(context);

        return context;
    }

    void SetCurrentContext(std::shared_ptr<IWindow> window,
                           std::shared_ptr<IGLContext> context) override
    {
        std::lock_guard<std::mutex> scopedX11Lock(gX11Lock);
        ScopedErrorHandler scopedErrors(ngXErrorHandler);

        const ngXWindow& xwindow = static_cast<const ngXWindow&>(*window);
        const ngXGLContext& xcontext = static_cast<const ngXGLContext&>(*context);

        glXMakeCurrent(mDisplay.mHandle, xwindow.mWindow.mHandle, xcontext.mHandle);
    }

};

std::shared_ptr<IWindowManager> CreateXWindowManager()
{
    if (!XInitThreads())
    {
        throw std::runtime_error("XInitThreads() failed");
    }

    std::lock_guard<std::mutex> scopedX11Lock(gX11Lock);
    ScopedErrorHandler scopedErrors(ngXErrorHandler);
    return std::shared_ptr<IWindowManager>(new ngXWindowManager(),
                                           ngXLockedDelete<ngXWindowManager>);
}

} // end namespace ng
