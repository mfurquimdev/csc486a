#include "ng/engine/emscripten/emscriptenwindow.hpp"

#include "ng/engine/egl/eglwindow.hpp"

#include "ng/engine/window/windowmanager.hpp"
#include "ng/engine/window/window.hpp"
#include "ng/engine/window/windowevent.hpp"
#include "ng/engine/util/debug.hpp"

#include <emscripten.h>

#include <queue>

namespace ng
{

class EmscriptenWindow : public IWindow
{
public:
    std::shared_ptr<IWindow> mEWindow;

    EmscriptenWindow(std::shared_ptr<IWindow> eWindow)
        : mEWindow(eWindow)
    { }

    void SwapBuffers() override
    {
        mEWindow->SwapBuffers();
    }

    void GetSize(int* width, int* height) const override
    {
        emscripten_get_canvas_size(width, height, nullptr);
    }

    void SetTitle(const char* title) override
    {
        // do nothing for now
    }

    const VideoFlags& GetVideoFlags() const override
    {
        return mEWindow->GetVideoFlags();
    }
};

class EmscriptenWindowManager;

static EmscriptenWindowManager* gEmWindowManager;

extern "C" {

void EMSCRIPTEN_KEEPALIVE OnEmKeyDown(int keyCode, int location);
void EMSCRIPTEN_KEEPALIVE OnEmKeyUp(int keyCode, int location);

void EMSCRIPTEN_KEEPALIVE OnEmMouseButtonDown(int button);
void EMSCRIPTEN_KEEPALIVE OnEmMouseButtonUp(int button);

} // end extern "C"

class EmscriptenWindowManager : public IWindowManager
{
    std::shared_ptr<IWindowManager> mEWindowManager;

    std::weak_ptr<IWindow> mWindow;

    int mWidth = 0;
    int mHeight = 0;

    std::queue<WindowEvent> mEventQueue;

public:
    EmscriptenWindowManager(std::shared_ptr<IWindowManager> eWindowManager)
        : mEWindowManager(std::move(eWindowManager))
    {
        if (gEmWindowManager)
        {
            throw std::logic_error("Emscripten does not allow multiple window managers");
        }

        gEmWindowManager = this;

        // inline javascript to set up the event callbacks
        EM_ASM(
            OnEmKeyDown = Module.cwrap('OnEmKeyDown', 'void', ['number', 'number']);
            OnEmKeyUp = Module.cwrap('OnEmKeyUp', 'void', ['number', 'number']);

            OnEmMouseButtonDown = Module.cwrap('OnEmMouseButtonDown', 'void', ['number']);
            OnEmMouseButtonUp = Module.cwrap('OnEmMouseButtonUp', 'void', ['number']);

            function receiveEvent(event) {
                switch (event.type) {
                case 'keydown':
                    event.preventDefault();
                    OnEmKeyDown(event.keyCode, event.location);
                    break;
                case 'keyup':
                    event.preventDefault();
                    OnEmKeyUp(event.keyCode, event.location);
                    break;
                case 'mousedown':
                    OnEmMouseButtonDown(event.button);
                    break;
                case 'mouseup':
                    OnEmMouseButtonUp(event.button);
                    break;
                default:
                    break;
                }
            }
            document.addEventListener('keydown', receiveEvent);
            document.addEventListener('keyup', receiveEvent);
            document.addEventListener('mousedown', receiveEvent);
            document.addEventListener('mouseup', receiveEvent);
        );
    }

    std::shared_ptr<IWindow> CreateWindow(const char* title,
                                          int width, int height,
                                          int x, int y,
                                          const VideoFlags& flags) override
    {

        std::shared_ptr<IWindow> win = std::make_shared<EmscriptenWindow>(
                    mEWindowManager->CreateWindow(title, width, height, x, y, flags));

        emscripten_set_canvas_size(width, height);
        mWidth = width;
        mHeight = height;

        mWindow = win;

        return win;
    }

    static Scancode MapJSKeycodeToScancode(int keyCode, int location)
    {
        enum {
            DOM_KEY_LOCATION_LEFT = 0x01,
            DOM_KEY_LOCATION_RIGHT = 0x02
        };

        if (keyCode >= 48 && keyCode <= 57)
        {
            return keyCode == 48 ? Scancode::Zero
                                 : Scancode(std::uint8_t(Scancode::One) + (keyCode - 49));
        }

        if (keyCode >= 65 && keyCode <= 90)
        {
            return Scancode(std::uint8_t(Scancode::A) + (keyCode - 65));
        }

        if (keyCode >= 96 && keyCode <= 105)
        {
            return keyCode == 96 ? Scancode::Keypad0
                                 : Scancode(std::uint8_t(Scancode::Keypad1) + (keyCode - 97));
        }

        if (keyCode >= 112 && keyCode <= 123)
        {
            return Scancode(std::uint8_t(Scancode::F1) + (keyCode - 112));
        }

        switch (keyCode)
        {
        case 8: return Scancode::Backspace;
        case 9: return Scancode::Tab;
        case 13: return Scancode::Enter;
        case 16: return location == DOM_KEY_LOCATION_LEFT ?
                        Scancode::LeftShift : Scancode::RightShift;
        case 17: return location == DOM_KEY_LOCATION_LEFT ?
                        Scancode::LeftControl : Scancode::RightControl;
        case 18: return location == DOM_KEY_LOCATION_LEFT ?
                        Scancode::LeftAlt : Scancode::RightAlt;
        case 19: return Scancode::Pause;
        case 20: return Scancode::CapsLock;
        case 27: return Scancode::Esc;
        case 32: return Scancode::Space;
        case 33: return Scancode::PageUp;
        case 34: return Scancode::PageDown;
        case 35: return Scancode::End;
        case 36: return Scancode::Home;
        case 37: return Scancode::LeftArrow;
        case 38: return Scancode::UpArrow;
        case 39: return Scancode::RightArrow;
        case 40: return Scancode::DownArrow;
        case 45: return Scancode::Insert;
        case 46: return Scancode::Delete;
        case 59: return Scancode::Semicolon;
        case 61: return Scancode::Equals;
        case 91: return Scancode::LeftGUI;
        case 92: return Scancode::RightGUI;
        case 93: return Scancode::Select;
        case 106: return Scancode::KeypadTimes;
        case 107: return Scancode::KeypadPlus;
        case 109: return Scancode::KeypadMinus;
        case 110: return Scancode::KeypadPeriod;
        case 111: return Scancode::KeypadSlash;
        case 144: return Scancode::NumLock;
        case 145: return Scancode::ScrollLock;
        case 173: return Scancode::Minus;
        case 186: return Scancode::Semicolon;
        case 187: return Scancode::KeypadEqualSign;
        case 188: return Scancode::Comma;
        case 190: return Scancode::Period;
        case 191: return Scancode::Slash;
        case 192: return Scancode::GraveAccent;
        case 219: return Scancode::LeftBracket;
        case 220: return Scancode::Backslash;
        case 221: return Scancode::RightBracket;
        case 222: return Scancode::SingleQuote;
        default:
            DebugPrintf("Unhandled keycode: %d\n", keyCode);
            return Scancode::Unknown;
        }
    }

    void OnKeyEvent(int keyCode, int location, KeyState state)
    {
        WindowEvent e;
        e.KeyPress.Type = WindowEventType::KeyPress;
        e.KeyPress.State = state;
        e.KeyPress.Scancode = MapJSKeycodeToScancode(keyCode, location);
        mEventQueue.push(e);
    }

    void OnMouseButtonEvent(int button, KeyState state)
    {
        WindowEvent e;
        e.Button.Type = WindowEventType::MouseButton;
        e.Button.Button = MouseButton(button);
        e.Button.State = state;

        // hack
        e.Button.X = e.Button.Y = 0;

        // hack
        std::fill(e.Button.ButtonStates, e.Button.ButtonStates + int(MouseButton::NumButtons), false);
        e.Button.ButtonStates[button] = true;
    }

    bool PollEvent(WindowEvent& we) override
    {
        WindowEvent e;
        e.Source = mWindow;

        // maybe this should be somewhere else?
        int width, height;
        emscripten_get_canvas_size(&width, &height, nullptr);
        if (width != mWidth || height != mHeight)
        {
            e.WindowStructure.Type = WindowEventType::WindowStructure;
            e.WindowStructure.X = 0; // maybe X and Y can somehow be found?
            e.WindowStructure.Y = 0;
            e.WindowStructure.Width = width;
            e.WindowStructure.Height = height;
            we = e;
            mWidth = width;
            mHeight = height;
            return true;
        }

        if (mEventQueue.size() > 0)
        {
            we = mEventQueue.front();
            mEventQueue.pop();
            return true;
        }

        return false;
    }

    std::shared_ptr<IGLContext> CreateContext(const VideoFlags& flags,
                                              std::shared_ptr<IGLContext> sharedWith) override
    {
        return mEWindowManager->CreateContext(flags, sharedWith);
    }

    void SetCurrentContext(std::shared_ptr<IWindow> window,
                           std::shared_ptr<IGLContext> context) override
    {
        const std::shared_ptr<IWindow>& eWindow = static_cast<const EmscriptenWindow&>(*window).mEWindow;
        mEWindowManager->SetCurrentContext(eWindow, context);
    }
};

extern "C" {

void EMSCRIPTEN_KEEPALIVE OnEmKeyDown(int keyCode, int location)
{
    gEmWindowManager->OnKeyEvent(keyCode, location, KeyState::Pressed);
}

void EMSCRIPTEN_KEEPALIVE OnEmKeyUp(int keyCode, int location)
{
    gEmWindowManager->OnKeyEvent(keyCode, location, KeyState::Released);
}

void EMSCRIPTEN_KEEPALIVE OnEmMouseButtonDown(int button)
{
    gEmWindowManager->OnMouseButtonEvent(button, KeyState::Pressed);
}

void EMSCRIPTEN_KEEPALIVE OnEmMouseButtonUp(int button)
{
    gEmWindowManager->OnMouseButtonEvent(button, KeyState::Released);
}

} // end extern "C"

std::shared_ptr<IWindowManager> CreateEmscriptenWindowManager(std::shared_ptr<IWindowManager> eWindowManager)
{
    return std::make_shared<EmscriptenWindowManager>(std::move(eWindowManager));
}

} // end namespace ng
