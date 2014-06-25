#ifndef NG_WINDOW_HPP
#define NG_WINDOW_HPP

namespace ng
{

class IGLContext;
struct VideoFlags;

class IWindow
{
public:
    virtual ~IWindow() = default;

    virtual void SwapBuffers() = 0;

    virtual void GetSize(int* width, int* height) const = 0;

    virtual void SetTitle(const char* title) = 0;

    int GetWidth() const
    {
        int w;
        GetSize(&w, nullptr);
        return w;
    }

    int GetHeight() const
    {
        int h;
        GetSize(nullptr, &h);
        return h;
    }

    virtual const VideoFlags& GetVideoFlags() const = 0;
};

} // end namespace ng

#endif // NG_WINDOW_HPP
