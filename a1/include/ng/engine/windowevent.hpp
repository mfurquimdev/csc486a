#ifndef NG_WINDOWEVENT_HPP
#define NG_WINDOWEVENT_HPP

#include <ostream>

namespace ng
{

class IWindow;

enum class WindowEventType : int
{
    Quit,
    MouseMotion,
    MouseButton
};

std::ostream& operator<<(std::ostream& os, WindowEventType);

enum class MouseButton : int
{
    Left,
    Middle,
    Right
};

std::ostream& operator<<(std::ostream& os, MouseButton);

enum class KeyState : int
{
    Pressed,
    Released
};

using ButtonState = KeyState;

std::ostream& operator<<(std::ostream& os, KeyState);

struct MouseMotionEvent
{
    WindowEventType type;
    int x;
    int y;
};

struct MouseButtonEvent
{
    WindowEventType type;
    MouseButton button;
    ButtonState state;
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
        MouseButtonEvent button;
    };
};

} // end namespace ng

#endif // NG_WINDOWEVENT_HPP
