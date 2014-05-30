#include "ng/engine/egl/eglwindow.hpp"

#include "ng/engine/window/windowmanager.hpp"

#include <EGL/egl.h>

#include <vector>

namespace ng
{

constexpr const char* ngEGLErrorToString(EGLint e)
{
    return e == EGL_SUCCESS ? "EGL_SUCCESS"
         : e == EGL_NOT_INITIALIZED ? "EGL_NOT_INITIALIZED"
         : e == EGL_BAD_ACCESS ? "EGL_BAD_ACCESS"
         : e == EGL_BAD_ALLOC ? "EGL_BAD_ALLOC"
         : e == EGL_BAD_ATTRIBUTE ? "EGL_BAD_ATTRIBUTE"
         : e == EGL_BAD_CONTEXT ? "EGL_BAD_CONTEXT"
         : e == EGL_BAD_CONFIG ? "EGL_BAD_CONFIG"
         : e == EGL_BAD_CURRENT_SURFACE ? "EGL_BAD_CURRENT_SURFACE"
         : e == EGL_BAD_DISPLAY ? "EGL_BAD_DISPLAY"
         : e == EGL_BAD_SURFACE ? "EGL_BAD_SURFACE"
         : e == EGL_BAD_MATCH ? "EGL_BAD_MATCH"
         : e == EGL_BAD_PARAMETER ? "EGL_BAD_PARAMETER"
         : e == EGL_BAD_NATIVE_PIXMAP ? "EGL_BAD_NATIVE_PIXMAP"
         : e == EGL_BAD_NATIVE_WINDOW ? "EGL_BAD_NATIVE_WINDOW"
         : e == EGL_CONTEXT_LOST ? "EGL_CONTEXT_LOST"
         : throw std::logic_error("Unknown EGL error code");
}

static void CheckEGLErrors()
{
    EGLint err = eglGetError();
    if (err != EGL_SUCCESS)
    {
        throw std::runtime_error(ngEGLErrorToString(err));
    }
}

class ngEGLDisplay
{
public:
    EGLDisplay mDisplay;

    ngEGLDisplay(NativeWindowType nativeDisplay)
    {
        mDisplay = eglGetDisplay(nativeDisplay);
        if (mDisplay == EGL_NO_DISPLAY)
        {
            throw std::runtime_error("No display connection matching nativeDisplay is available");
        }

        if (eglInitialize(mDisplay, NULL, NULL) == EGL_FALSE)
        {
            CheckEGLErrors();
        }
    }

    ~ngEGLDisplay()
    {
        eglTerminate(mDisplay);
    }
};

class ngEWindow : public IWindow
{
public:
    VideoFlags mVideoFlags;
    EGLDisplay mDisplay;
    EGLSurface mSurface;

    ngEWindow(const VideoFlags& videoFlags,
              EGLDisplay dpy,
              EGLConfig config,
              EGLNativeWindowType nativeWindow,
              const EGLint* attribList)
        : mVideoFlags(videoFlags)
        , mDisplay(dpy)
    {
        mSurface = eglCreateWindowSurface(mDisplay, config, nativeWindow, attribList);
        if (mSurface == EGL_NO_SURFACE)
        {
            CheckEGLErrors();
        }
    }

    ~ngEWindow()
    {
        eglDestroySurface(mDisplay, mSurface);
    }

    void SwapBuffers() override
    {
        eglSwapBuffers(mDisplay->mDisplay, mSurface);
    }

    void GetSize(int* width, int* height) const override
    {
        throw std::logic_error("EGL does not support GetSize()");
    }

    void SetTitle(const char*) override
    {
        throw std::logic_error("EGL does not support SetTitle()");
    }

    const VideoFlags& GetVideoFlags() const override
    {
        return mVideoFlags;
    }
};

class ngEGLContext : public IGLContext
{
public:
    EGLDisplay mDisplay;
    EGLContext mContext;

    ngEGLContext(EGLDisplay display,
                 EGLConfig config,
                 EGLContext shareContext,
                 const EGLint* attribList)
        : mDisplay(display)
    {
        mContext = eglCreateContext(mDisplay, config, shareContext, attribList);
        if (mContext == EGL_NO_CONTEXT)
        {
            CheckEGLErrors();
        }
    }

    ~ngEGLContext()
    {
        eglDestroyContext(mDisplay, mContext);
    }

    bool IsExtensionSupported(const char* extension) override
    {
        const char *extList = eglQueryString(mDisplay, EGL_EXTENSIONS);

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

    void* GetProcAddress(const char* proc) override
    {
        return eglGetProcAddress(proc);
    }
};

class ngEWindowManager : public IWindowManager
{
    EGLDisplay mDisplay;
    EGLNativeWindowType mNativeWindow;

public:
    ngEWindowManager(EGLNativeWindowType nativeWindow)
        : mDisplay(nativeWindow)
        , mNativeWindow(nativeWindow)
    { }

    static std::vector<EGLint> VideoFlagsToAttribList(const VideoFlags& flags)
    {
        return
        {
            EGL_RED_SIZE        , flags.RedSize,
            EGL_GREEN_SIZE      , flags.GreenSize,
            EGL_BLUE_SIZE       , flags.BlueSize,
            EGL_ALPHA_SIZE      , flags.AlphaSize,
            EGL_DEPTH_SIZE      , flags.DepthSize,
            EGL_STENCIL_SIZE    , flags.StencilSize,
            //EGL_SAMPLE_BUFFERS  , 1,
            //EGL_SAMPLES         , 4,
            EGL_NONE
        };
    }

    static EGLConfig GetBestEGLConfig(EGLDisplay* display, const EGLint* attribList)
    {
        EGLint numConfigs;
        if (eglGetConfigs(mDisplay.mHandle, NULL, 0, &numConfigs) == EGL_FALSE)
        {
            CheckEGLErrors();
        }

        std::vector<EGLConfig> configs(numConfigs);

        EGLint numChosenConfigs;
        if (eglChooseConfig(mDisplay.mHandle, attribList, configs.data(), configs.size(), &numChosenConfigs) == EGL_FALSE)
        {
            CheckEGLErrors();
        }

        // pick the config with the most samples per pixel
        int bestConfigIndex = -1, bestNumSamples = -1;
        for (int i = 0; i < numChosenConfigs; i++)
        {
            EGLint sampleBuffers;
            EGLint numSamples;

            EGLBoolean sampleBufferStatus = eglGetConfigAttrib(mDisplay.mHandle, configs[i], EGL_SAMPLE_BUFFERS, &sampleBuffers);
            EGLBoolean numSamplesStatus = eglGetConfigAttrib(mDisplay.mHandle, configs[i], GLX_SAMPLES, &numSamples);

            if (sampleBufferStatus == EGL_FALSE || numSamplesStatus == EGL_FALSE)
            {
                CheckEGLErrors();
            }

            if (bestConfigIndex < 0 || (sampleBuffers && numSamples > bestNumSamples))
            {
                bestConfigIndex = i;
                bestNumSamples = numSamples;
            }
        }

        if (bestConfigIndex == -1)
        {
            throw std::runtime_error("No best EGLConfig");
        }

        return configs[bestConfigIndex];
    }

    std::shared_ptr<IWindow> CreateWindow(const char*,
                                          int, int,
                                          int, int,
                                          const VideoFlags& flags) override
    {
        std::vector<EGLint> attribVector = VideoFlagsToAttribList(flags);
        EGLint* attribList = attribVector.data();

        EGLConfig bestConfig = GetBestEGLConfig(mDisplay.mHandle, attribList);

        return std::make_shared<ngEWindow>(flags, mDisplay.mHandle, bestConfig, mNativeWindow, NULL);
    }

    bool PollEvent(WindowEvent& we) override
    {
        throw std::logic_error("EWindowManager does not support PollEvent()");
    }

    std::shared_ptr<IGLContext> CreateContext(const VideoFlags& flags,
                                              std::shared_ptr<IGLContext> sharedWith) override
    {
        EGLContext shareContext = EGL_NO_CONTEXT;
        if (sharedWith)
        {
            const ngEGLContext& toShareWith = static_cast<const ngEGLContext&>(*sharedWith);
            shareContext = toShareWith.mContext;
        }

        std::vector<EGLint> configAttribVector = VideoFlagsToAttribList(flags);
        EGLint* configAttribList = attribVector.data();

        EGLConfig bestConfig = GetBestEGLConfig(mDisplay.mHandle, configAttribList);

        const int contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2 };

        return std::make_shared<ngEGLContext>(new ngEGLContext(mDisplay.mHandle, bestConfig, shareContext, contextAttribs));
    }

    void SetCurrentContext(std::shared_ptr<IWindow> window,
                           std::shared_ptr<IGLContext> context) override
    {
        const ngEWindow& ewindow = static_cast<const ngEWindow&>(*window);
        const ngEGLContext& econtext = static_cast<const ngEGLContext&>(*context);

        eglMakeCurrent(mDisplay.mHandle, ewindow.mSurface, ewindow.mSurface, econtext.mContext);
    }
};

std::shared_ptr<IWindowManager> CreateEWindowManager(EGLNativeWindowType nativeWindow)
{
    return std::make_shared<ngEWindowManager>(nativeWindow);
}

} // end namespace ng
