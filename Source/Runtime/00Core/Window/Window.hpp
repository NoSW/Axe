#pragma once

#include <00Core/Log/Log.hpp>

struct SDL_Window;
struct SDL_WindowEvent;

namespace axe::window
{
class Window final
{
public:
    bool init(const char* title) noexcept;
    void exit() noexcept;
    bool load() noexcept { return true; }
    bool unload() noexcept { return true; }
    bool update(float deltaTime) noexcept;

    bool isQuit() const { return _mQuit; }
    bool isResized() const { return _mResized; }
    bool isMinimized() const { return _mMinimized; }
    u32 width() const { return _mWidth; }
    u32 height() const { return _mHeight; }

    SDL_Window* handle() { return _mpSDLWindow; }

private:
    void _pollWindowEvent(SDL_WindowEvent* windowEvent) noexcept;
    void _pollKeyboardEvent() noexcept {}
    void _pollMouseEvent() noexcept {}

    u32 _mWidth                 = 800;
    u32 _mHeight                = 600;
    u32 _mResizable         : 1 = 1;
    u32 _mResized           : 1 = 0;
    u32 _mMaximized         : 1 = 0;
    u32 _mMinimized         : 1 = 0;
    u32 _mQuit              : 1 = 0;
    u32 _mUseExternalWindow : 1 = 0;
    u32 _mFocused           : 1 = 1;

    SDL_Window* _mpSDLWindow    = nullptr;
};

}  // namespace axe::window
