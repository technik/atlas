#include "BaseApp.h"

#include <iostream>
#include <input/keyboard.h>
#include <input/mouse.h>

BaseApp::BaseApp()
{}

BaseApp::~BaseApp()
{
}

bool BaseApp::init()
{
    m_window = std::shared_ptr<Window>(Window::CreateNewWindow("DX12 renderer", m_ClientWidth, m_ClientHeight));
    if (!m_window)
    {
        std::cout << "Failed to create window\n";
        return false;
    }
    m_window->setFullScreen(m_fullScreen);

    gfx::DX12Context::Init();

    // Initialize the global window rect variable.
    m_swapChain = std::shared_ptr<SwapChain>(SwapChain::CreateSwapChain(m_window.get(), m_ClientWidth, m_ClientHeight, 2));

    ::ShowWindow(m_window->m_hWnd, SW_SHOW);

    input::Keyboard::init();
    input::Mouse::init();

    return true;
}

void BaseApp::end()
{
    // Flush graphics before destroying the swapchain
    gfx::RenderContext()->graphicsQueue().flush();

    // Ensure correct destruction order by deinitializing in the reverse order things were initialized
    m_swapChain = nullptr;
    gfx::DX12Context::Deinit();
    m_window = nullptr;
}

bool BaseApp::processWindowMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
    {
        bool alt = (::GetAsyncKeyState(VK_MENU) & 0x8000) != 0;

        switch (wParam)
        {
        case 'V':
            m_VSync = !m_VSync;
            m_swapChain->setVSync(m_VSync);
            break;
        case VK_ESCAPE:
            ::PostQuitMessage(0);
            break;
        case VK_F11:
            m_window->setFullScreen(!m_window->isFullScreen());
            break;
        case VK_RETURN:
            if (alt)
            {
                m_window->setFullScreen(!m_window->isFullScreen());
            }
            break;
            break;
        }
    }
    break;
    // The default window procedure will play a system notification sound 
    // when pressing the Alt+Enter keyboard combination if this message is 
    // not handled.
    case WM_SYSCHAR:
        break;
    case WM_SIZE:
    {
        RECT clientRect = {};
        ::GetClientRect(hwnd, &clientRect);

        int width = clientRect.right - clientRect.left;
        int height = clientRect.bottom - clientRect.top;

        resize(width, height);
    }
    break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        break;
    default:
        return false;
    }

    return true;
}

bool BaseApp::pollWindowMessages()
{
    MSG msg = {};

    input::Keyboard::get()->refresh();

    while (msg.message != WM_QUIT)
    {
        if (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            ::TranslateMessage(&msg);

            if (input::Keyboard::get()->processWin32Message(msg))
            {
                continue;
            }
            if (input::Mouse::get()->processMessage(msg))
            {
                continue;
            }

            ::DispatchMessage(&msg);
        }
        else
        {
            break;
        }
    }
    return msg.message != WM_QUIT;
}

void BaseApp::resize(uint32_t width, uint32_t height)
{
    if (m_ClientWidth != width || m_ClientHeight != height)
    {
        // Don't allow 0 size swap chain back buffers.
        m_ClientWidth = std::max(1u, width);
        m_ClientHeight = std::max(1u, height);

        // Flush the GPU queue to make sure the swap chain's back buffers
        // are not being referenced by an in-flight command list.
        gfx::RenderContext()->graphicsQueue().flush();

        m_swapChain->Resize(m_ClientWidth, m_ClientHeight);
    }
}