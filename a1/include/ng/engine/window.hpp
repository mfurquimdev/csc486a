#ifndef NG_WINDOW_HPP
#define NG_WINDOW_HPP

#include <memory>

namespace ng
{

class IGLContext;

class IWindow
{
public:
    virtual ~IWindow() = default;

    virtual void SwapBuffers() = 0;

    virtual void GetSize(int* width, int* height) = 0;

    virtual std::unique_ptr<IGLContext> CreateContext() = 0;

    virtual void MakeCurrent(const IGLContext& context) = 0;
};

struct WindowFlags
{
    int RedSize = 8;
    int GreenSize = 8;
    int BlueSize = 8;
    int AlphaSize = 8;
    int DepthSize = 24;
    int StencilSize = 8;

    bool DoubleBuffered = true;
};

std::unique_ptr<IWindow> CreateWindow(const char* title,
                                      int width, int height,
                                      int x = 0, int y = 0,
                                      const WindowFlags& flags = WindowFlags());

} // end namespace ng

#endif // NG_WINDOW_HPP
