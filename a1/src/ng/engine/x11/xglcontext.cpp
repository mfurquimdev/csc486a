#include "ng/engine/x11/xglcontext.hpp"

#include "ng/engine/x11/xerrorhandler.hpp"

#include <X11/Xlib.h>

#include <stdexcept>
#include <cassert>
#include <cstring>

namespace ng
{

ngXGLContext::ngXGLContext(Display* dpy, GLXFBConfig fbConfig)
    : mDisplay(dpy)
{
    using glXCreateContextAttribsARBProc = GLXContext(*)(Display*, GLXFBConfig, GLXContext, Bool, const int*);

    glXCreateContextAttribsARBProc glXCreateContextAttribsARB = 0;
    glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc) GetProcAddress("glXCreateContextAttribsARB");

    if (!IsExtensionSupported("GLX_ARB_create_context") ||
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

bool ngXGLContext::IsExtensionSupported(const char* extension)
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

void* ngXGLContext::GetProcAddress(const char *proc)
{
    return (void*) glXGetProcAddressARB((const GLubyte*) proc);
}

} // end namespace ng
