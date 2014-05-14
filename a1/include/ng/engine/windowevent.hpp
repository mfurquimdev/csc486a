#ifndef NG_WINDOWEVENT_HPP
#define NG_WINDOWEVENT_HPP

namespace ng
{

class IWindow;

enum class WindowEventType
{
    Quit,
    Motion
};

struct MouseMotionEvent
{
    WindowEventType type;
    int x;
    int y;
};

struct WindowEvent
{
    IWindow* source;

    union
    {
        WindowEventType type;
        MouseMotionEvent motion;
    };
};

} // end namespace ng

#endif // NG_WINDOWEVENT_HPP
