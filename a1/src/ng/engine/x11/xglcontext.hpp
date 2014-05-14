#ifndef NG_XGLCONTEXT_HPP
#define NG_XGLCONTEXT_HPP

#include <memory>

namespace ng
{

class IGLContext;

std::unique_ptr<IGLContext> CreateXGLContext(const int* attribList);

} // end namespace ng

#endif // NG_XGLCONTEXT_HPP
