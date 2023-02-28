#include "00Core/Window/Window.hpp"
#include <SDL.h>

namespace axe::window
{
bool Window::init(const char* title) noexcept
{
    bool succ = SDL_Init(SDL_INIT_EVERYTHING) == 0;
    AXE_ASSERT(succ);
    u32 sdlWindowFlag = SDL_WINDOW_SHOWN;
    sdlWindowFlag |= _mResizable ? SDL_WINDOW_RESIZABLE : 0;
    sdlWindowFlag |= /* TODO */ SDL_WINDOW_VULKAN;
    _mpSDLWindow = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, _mWidth, _mHeight, sdlWindowFlag);

    if (_mMaximized) { SDL_MaximizeWindow(_mpSDLWindow); }
    SDL_GL_GetDrawableSize(_mpSDLWindow, (i32*)(&_mWidth), (i32*)(&_mHeight));

    return _mpSDLWindow != nullptr;
}

void Window::exit() noexcept
{
    SDL_DestroyWindow(_mpSDLWindow);
    SDL_Quit();
}

bool Window::update(float deltaTime) noexcept
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_QUIT: _mQuit = true; break;
            case SDL_WINDOWEVENT: _pollWindowEvent(&(event.window)); break;
            default: break;
        }
    }
    return true;
}

void Window::_pollWindowEvent(SDL_WindowEvent* windowEvent) noexcept
{
    switch (windowEvent->event)
    {
        case SDL_WINDOWEVENT_RESIZED:
            _mResized = true;
            _mWidth   = windowEvent->data1;
            _mHeight  = windowEvent->data2;
            break;
        case SDL_WINDOWEVENT_MINIMIZED: _mMinimized = true; break;
        case SDL_WINDOWEVENT_MAXIMIZED: _mMaximized = true; break;
        case SDL_WINDOWEVENT_RESTORED: _mMaximized = _mMinimized = false; break;
        default: break;
    }
}

}  // namespace axe::window