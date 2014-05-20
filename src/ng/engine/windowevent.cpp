#include "ng/engine/windowevent.hpp"

namespace ng
{

std::ostream& operator<<(std::ostream& os, WindowEventType e)
{
    switch (e)
    {
    case WindowEventType::Quit: os << "Quit"; break;
    case WindowEventType::MouseMotion: os << "MouseMotion"; break;
    case WindowEventType::MouseButton: os << "MouseButton"; break;
    default: os << "???";
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, MouseButton b)
{
    switch (b)
    {
    case MouseButton::Left: os << "Left"; break;
    case MouseButton::Middle: os << "Middle"; break;
    case MouseButton::Right: os << "Right"; break;
    default: os << "???";
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, KeyState ks)
{
    switch (ks)
    {
    case KeyState::Pressed: os << "Pressed"; break;
    case KeyState::Released: os << "Released"; break;
    default: os << "???";
    }
    return os;
}

} // end namespace ng
