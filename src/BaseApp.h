#pragma once

#include "gfx/Window.h"
#include "gfx/DX12Context.h"
#include "gfx/swapChain.h"

class BaseApp
{
protected:
    virtual bool init();

    virtual bool update() = 0;
    virtual void render() = 0;

    auto window() const { return m_window; }
    auto swapChain() const { return m_swapChain; }

    uint32_t m_ClientWidth = 1280;
    uint32_t m_ClientHeight = 720;

    // By default, use windowed mode.
    // Can be toggled with the Alt+Enter or F11
    bool m_fullScreen = false;

public:
    virtual void end();

    BaseApp();
    ~BaseApp();

    virtual void resize(uint32_t width, uint32_t height);
    bool processWindowMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    bool pollWindowMessages();

private:
    std::shared_ptr<SwapChain> m_swapChain;
    bool m_VSync = true;

    std::shared_ptr<Window> m_window;
};