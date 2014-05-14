#ifndef NG_XGLCONTEXT_HPP
#define NG_XGLCONTEXT_HPP

#include "ng/engine/x11/xdisplay.hpp"

#include "ng/engine/glcontext.hpp"

#include <GL/glx.h>

namespace ng
{

class ngXGLContext : public IGLContext
{
public:
    Display* mDisplay;
    GLXContext mHandle;

    ngXGLContext(Display* dpy, GLXFBConfig config);
    ~ngXGLContext();

    bool IsExtensionSupported(const char *extension) override;

    void* GetProcAddress(const char *proc) override;
};

} // end namespace ng

#endif // NG_XGLCONTEXT_HPP
