#include "ng/engine/x11/xglcontext.hpp"

#include "ng/engine/x11/xglcontextimpl.hpp"

#include "ng/engine/x11/xerrorhandler.hpp"

namespace ng
{

std::unique_ptr<IGLContext> CreateXGLContext(const int* attribList)
{
    XSetErrorHandler(ngXErrorHandler);
    return std::unique_ptr<IGLContext>(new ngXGLContext(attribList));
}

} // end namespace ng
