#include "ng/engine/x11/xglcontext.hpp"

#include "ng/engine/x11/xerrorhandler.hpp"
#include "ng/engine/x11/glxextensions.hpp"

#include <X11/Xlib.h>

#include <cassert>
#include <stdexcept>

namespace ng
{

ngXGLContext::ngXGLContext(Display* dpy, GLXFBConfig fbConfig)
    : mDisplay(dpy)
{
    using glXCreateContextAttribsARBProc = GLXContext(*)(Display*, GLXFBConfig, GLXContext, Bool, const int*);

    const char *glxExts = glXQueryExtensionsString(dpy, DefaultScreen(dpy));

    glXCreateContextAttribsARBProc glXCreateContextAttribsARB = 0;
    glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc)
            glXGetProcAddressARB((const GLubyte*) "glXCreateContextAttribsARB");

    if (!IsExtensionSupported(glxExts, "GLX_ARB_create_context") ||
        !glXCreateContextAttribsARB)
    {
        // glXCreateContextAttribsARB() not found
        // use old style GLX context
        mHandle = glXCreateNewContext(dpy, fbConfig, GLX_RGBA_TYPE, 0, True);
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

        mHandle = glXCreateContextAttribsARB(dpy, fbConfig, 0, True, contextAttribs);

        // flush errors
        XSync(dpy, False);
        if (sFailedToBuildContext || !mHandle)
        {
            // Couldn't create a GL 3.0 context, so fall back to 2.x context.
            contextAttribs[1] = 1;
            contextAttribs[3] = 0;

            mHandle = glXCreateContextAttribsARB(dpy, fbConfig, 0, True, contextAttribs);
            XSync(dpy, False);
        }
    }

    if (!mHandle)
    {
        // hopefully X11's error reporting catches errors before this does.
        throw std::runtime_error("Failed to build context");
    }
}

ngXGLContext::~ngXGLContext()
{
    glXDestroyContext(mDisplay, mHandle);
}

} // end namespace ng
