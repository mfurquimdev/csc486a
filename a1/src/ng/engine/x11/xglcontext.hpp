#ifndef NG_XGLCONTEXT_HPP
#define NG_XGLCONTEXT_HPP

#include "ng/engine/x11/xdisplay.hpp"
#include "ng/engine/x11/xvisualinfo.hpp"

#include "ng/engine/iglcontext.hpp"

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
};

} // end namespace ng

#endif // NG_XGLCONTEXT_HPP
