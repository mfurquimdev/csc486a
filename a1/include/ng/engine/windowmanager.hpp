#ifndef NG_WINDOWMANAGER_HPP
#define NG_WINDOWMANAGER_HPP

#include <memory>

namespace ng
{

class IWindow;
struct WindowEvent;

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

class IWindowManager
{
public:
    virtual ~IWindowManager() = default;

    std::shared_ptr<IWindow> CreateWindow(const char* title,
                                          int width, int height,
                                          int x = 0, int y = 0,
                                          const WindowFlags& flags = WindowFlags())
    {
        return CreateWindowImpl(title, width, height, x, y, flags);
    }

    bool PollEvent(WindowEvent& we)
    {
        return PollEventImpl(we);
    }

protected:
    virtual std::shared_ptr<IWindow> CreateWindowImpl(const char* title,
                                                      int width, int height,
                                                      int x, int y,
                                                      const WindowFlags& flags) = 0;

    virtual bool PollEventImpl(WindowEvent& we) = 0;
};

std::unique_ptr<IWindowManager> CreateWindowManager();

} // end namespace ng

#endif // NG_WINDOWMANAGER_HPP
