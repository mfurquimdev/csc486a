#ifndef NG_WINDOWMANAGER_HPP
#define NG_WINDOWMANAGER_HPP

#include <memory>

namespace ng
{

class IWindow;
class IGLContext;
struct WindowEvent;

struct VideoFlags
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

    virtual std::shared_ptr<IWindow> CreateWindow(const char* title,
                                                  int width, int height,
                                                  int, int y,
                                                  const VideoFlags& flags) = 0;

    virtual bool PollEvent(WindowEvent& we) = 0;

    virtual std::shared_ptr<IGLContext> CreateContext(const VideoFlags& flags) = 0;

    virtual void SetCurrentContext(std::shared_ptr<IWindow> window,
                                   std::shared_ptr<IGLContext> context) = 0;
};

std::shared_ptr<IWindowManager> CreateWindowManager();

} // end namespace ng

#endif // NG_WINDOWMANAGER_HPP
