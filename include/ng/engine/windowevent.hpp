#ifndef NG_WINDOWEVENT_HPP
#define NG_WINDOWEVENT_HPP

#include <memory>
#include <bitset>

namespace ng
{

class IWindow;

enum class MouseButton : int
{
    Left,
    Middle,
    Right,
    NumButtons
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

// according to USB standards
enum class Scancode : std::uint8_t
{
    Unknown = 0,

    A = 0x04,
    B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

    One,
    ExclamationMark = One,
    Two,
    At = Two,
    Three,
    Hash = Three,
    Four,
    Dollar = Four,
    Five,
    Percent = Five,
    Six,
    Caret = Six,
    Seven,
    Ampersand = Seven,
    Eight,
    Times = Eight,
    Nine,
    LeftParenthesis = Nine,
    Zero,
    RightParenthesis = Zero,

    Enter,
    Esc,
    Backspace,
    Tab,
    Space,

    Minus,
    Underscore = Minus,
    Plus,
    Equals = Plus,
    LeftBracket,
    LeftBrace = LeftBracket,
    RightBracket,
    RightBrace = RightBracket,
    Backslash,
    VerticalBar = Backslash,

    Europe1,

    Semicolon,
    Colon = Semicolon,
    SingleQuote,
    DoubleQuote = SingleQuote,
    GraveAccent,
    Tilde = GraveAccent,
    Comma,
    LessThan = Comma,
    Period,
    GreaterThan = Period,
    Slash,
    QuestionMark = Slash,
    CapsLock,

    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,

    PrintScreen,
    ScrollLock,
    Break,
    Pause = Break,

    Insert,
    Home,
    PageUp,
    Delete,
    End,
    PageDown,

    RightArrow,
    LeftArrow,
    DownArrow,
    UpArrow,

    NumLock,
    KeypadSlash,
    KeypadTimes,
    KeypadMinus,
    KeypadPlus,
    KeypadEnter,
    Keypad1,
    KeypadEnd = Keypad1,
    Keypad2,
    KeypadDown = Keypad2,
    Keypad3,
    KeypadPageDown = Keypad3,
    Keypad4,
    KeypadLeft = Keypad4,
    Keypad5,
    KeypadBegin = Keypad5,
    Keypad6,
    KeypadRight = Keypad6,
    Keypad7,
    KeypadHome = Keypad7,
    Keypad8,
    KeypadUp = Keypad8,
    Keypad9,
    KeypadPageUp = Keypad9,
    Keypad0,
    KeypadInsert = Keypad0,
    KeypadPeriod,
    KeypadDelete = KeypadPeriod,

    Europe2,
    App,
    Power,
    KeypadEqual,

    F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24,

    Execute,
    Help,
    Menu,
    Select,
    Stop,
    Again,
    Undo,
    Cut,
    Copy,
    Paste,
    Find,
    Mute,
    VolumeUp,
    VolumeDown,
    LockingCapsLock,
    LockingNumLock,
    LockingScrollLock,
    KeypadComma,
    KeypadEqualSign,

    International1,
    International2,
    International3,
    International4,
    International5,
    International6,
    International7,
    International8,
    International9,

    LANG1,
    LANG2,
    LANG3,
    LANG4,
    LANG5,
    LANG6,
    LANG7,
    LANG8,
    LANG9,

    AlternateErase,
    SysReqAttention,
    Cancel,
    Clear,
    Prior,
    Return,
    Separator,
    Out,
    Oper,
    ClearAgain,
    CrSelProps,
    ExSel,

    LeftControl,
    LeftShift,
    LeftAlt,
    LeftGUI,
    RightControl,
    RightShift,
    RightAlt,
    RightGUI
};

enum class WindowEventType
{
    Quit,
    MouseMotion,
    MouseButton,
    MouseScroll,
    KeyPress,
    KeyRelease,
    WindowStructure
};

constexpr const char* WindowEventTypeToString(WindowEventType et)
{
    return et == WindowEventType::Quit ? "Quit"
         : et == WindowEventType::MouseMotion ? "MouseMotion"
         : et == WindowEventType::MouseButton ? "MouseButton"
         : et == WindowEventType::MouseScroll ? "MouseScroll"
         : et == WindowEventType::KeyPress ? "KeyPress"
         : et == WindowEventType::KeyRelease ? "KeyRelease"
         : et == WindowEventType::WindowStructure ? "WindowStructure"
         : throw std::logic_error("No such WindowEventType");
}

struct MouseMotionEvent
{
    WindowEventType Type;

    int OldX;
    int OldY;

    int X;
    int Y;

    bool ButtonStates[int(MouseButton::NumButtons)];
};

struct MouseButtonEvent
{
    WindowEventType Type;
    MouseButton Button;
    ButtonState State;
    int X;
    int Y;

    bool ButtonStates[int(MouseButton::NumButtons)];
};

struct MouseScrollEvent
{
    WindowEventType Type;
    int Direction;
};

struct KeyEvent
{
    WindowEventType Type;
    KeyState State;
    ng::Scancode Scancode;
    // TODO: Add support for keycodes
};

struct WindowStructureEvent
{
    WindowEventType Type;
    int X;
    int Y;
    int Width;
    int Height;
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
        KeyEvent KeyPress;
        KeyEvent KeyRelease;
        WindowStructureEvent WindowStructure;
    };
};

} // end namespace ng

#endif // NG_WINDOWEVENT_HPP
