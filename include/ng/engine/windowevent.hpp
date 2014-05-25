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
    MouseButton,
    MouseScroll,
    KeyPress
};

constexpr const char* WindowEventTypeToString(WindowEventType et)
{
    return et == WindowEventType::Quit ? "Quit"
         : et == WindowEventType::MouseMotion ? "MouseMotion"
         : et == WindowEventType::MouseButton ? "MouseButton"
         : et == WindowEventType::MouseScroll ? "MouseScroll"
         : et == WindowEventType::KeyPress ? "KeyPress"
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
    WindowEventType Type;
    int X;
    int Y;
};

struct MouseButtonEvent
{
    WindowEventType Type;
    MouseButton Button;
    ButtonState State;
    int X;
    int Y;
};

struct MouseScrollEvent
{
    WindowEventType Type;
    int Direction;
};

struct KeyPressEvent
{
    WindowEventType Type;
};

struct WindowEvent
{
    std::weak_ptr<IWindow> Source;

    union
    {
        WindowEventType Type;
        MouseMotionEvent Motion;
        MouseButtonEvent Button;
        MouseScrollEvent Scroll;
    };
};

} // end namespace ng

#endif // NG_WINDOWEVENT_HPP
