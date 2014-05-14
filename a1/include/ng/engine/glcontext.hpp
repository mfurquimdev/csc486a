#ifndef NG_GLCONTEXT_HPP
#define NG_GLCONTEXT_HPP

namespace ng
{

class IGLContext
{
public:
    virtual ~IGLContext() = default;

    virtual bool IsExtensionSupported(const char* extension) = 0;

    virtual void* GetProcAddress(const char* proc) = 0;
};

} // end namespace ng

#endif // NG_GLCONTEXT_HPP
