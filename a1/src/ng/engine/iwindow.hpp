#ifndef NG_IWINDOW_HPP
#define NG_IWINDOW_HPP

namespace ng
{

class IGLContext;

class IWindow
{
public:
    virtual ~IWindow() = default;

    virtual void SwapBuffers() = 0;

    virtual void GetSize(int* width, int* height) = 0;

    virtual void MakeCurrent(const IGLContext& context) = 0;
};

} // end namespace ng

#endif // NG_IWINDOW_HPP
