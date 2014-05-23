#ifndef NG_WINDOWEVENT_HPP
#define NG_WINDOWEVENT_HPP

#include <memory>

namespace ng
{

class IWindow;

enum class WindowEventType
{
    Quit,
    MouseMotion,
    MouseButton
};

constexpr const char* WindowEventTypeToString(WindowEventType et)
{
    return et == WindowEventType::Quit ? "Quit"
         : et == WindowEventType::MouseMotion ? "MouseMotion"
         : et == WindowEventType::MouseButton ? "MouseButton"
         : throw std::logic_error("No such WindowEventType");
}

enum class MouseButton
{
    Left,
    Middle,
    Right
};

constexpr const char* MouseButtonToString(MouseButton mb)
{
    return mb == MouseButton::Left ? "Left"
         : mb == MouseButton::Middle ? "Middle"
         : mb == MouseButton::Right ? "Right"
         : throw std::logic_error("No such MouseButton");
}

enum class KeyState
{
    Pressed,
    Released
};

using ButtonState = KeyState;

constexpr const char* KeyStateToString(KeyState ks)
{
    return ks == KeyState::Pressed ? "Pressed"
         : ks == KeyState::Released ? "Released"
         : throw std::logic_error("No such KeyState");
}

constexpr const char* ButtonStateToString(ButtonState bs)
{
    return KeyStateToString(bs);
}

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
    std::weak_ptr<IWindow> source;

    union
    {
        WindowEventType type;
        MouseMotionEvent motion;
        MouseButtonEvent button;
    };
};

} // end namespace ng

#endif // NG_WINDOWEVENT_HPP
