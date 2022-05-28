#include "Window.h"
#include <iostream>
#include <cassert>

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

namespace {
	bool gIsWindowClassRegistered = false;
	const char* gWindowClassName = "WindowClass";

	//------------------------------------------------------------------------------------------
	bool registerClass(bool useCursor)
	{
		if (gIsWindowClassRegistered)
			return true;

        // Windows 10 Creators update adds Per Monitor V2 DPI awareness context.
        // Using this awareness context allows the client area of the window 
        // to achieve 100% scaling while still allowing non-client window content to 
        // be rendered in a DPI sensitive fashion.
        SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

		HINSTANCE processInstance = GetModuleHandle(NULL);

		// -- Register a new window class --
		WNDCLASSEX winClass = {};
		winClass.cbSize = sizeof(WNDCLASSEX);
		winClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW; // Class style
		winClass.lpfnWndProc = WndProc;
		winClass.hInstance = processInstance;
		winClass.hCursor = useCursor ? ::LoadCursor(NULL, IDC_ARROW) : NULL;
		winClass.hIcon = NULL;	// Default icon // ::LoadIcon(hInst, NULL);
		winClass.lpszClassName = gWindowClassName;

		if (!RegisterClassEx(&winClass))
		{
			auto error = GetLastError();
			std::cout << "Failed to register window class. Error code: " << error << "\n";
			return false;
		}
		gIsWindowClassRegistered = true;
		return true;
	}
}

Window* Window::CreateNewWindow(const char* windowName, uint32_t width, uint32_t height)
{
	const bool showCursor = true;
	if (!registerClass(showCursor))
	{
		return nullptr;
	}

	int screenWidth = ::GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = ::GetSystemMetrics(SM_CYSCREEN);

	RECT windowRect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
	::AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	int windowWidth = windowRect.right - windowRect.left;
	int windowHeight = windowRect.bottom - windowRect.top;

	// Center the window within the screen. Clamp to 0, 0 for the top-left corner.
	int windowX = std::max<int>(0, (screenWidth - windowWidth) / 2);
	int windowY = std::max<int>(0, (screenHeight - windowHeight) / 2);

	HWND hWnd = ::CreateWindowEx(
		NULL,
		gWindowClassName,
		windowName,
		WS_OVERLAPPEDWINDOW,
		windowX,
		windowY,
		windowWidth,
		windowHeight,
		NULL,
		NULL,
		NULL,
		nullptr
	);

	assert(hWnd && "Failed to create window");

	// Resize client area to match the requested size
	{
		RECT rcClient;
		POINT ptDiff;
		GetClientRect(hWnd, &rcClient);
		ptDiff.x = windowWidth - rcClient.right;
		ptDiff.y = windowHeight - rcClient.bottom;
		MoveWindow(hWnd, windowX, windowY, windowWidth + ptDiff.x, windowHeight + ptDiff.y, TRUE);
		// Note: Maybe we could do this using SM_CYCAPTION and SM_CYBORDER instead of resizing a window.
	}

	auto newWindow = new Window();
	newWindow->m_hWnd = hWnd;

	::GetWindowRect(newWindow->m_hWnd, &newWindow->m_WindowRect);

	return newWindow;
}

void Window::setFullScreen(bool fullscreen)
{
	if (m_fullScreen != fullscreen)
	{
		m_fullScreen = fullscreen;

		if (m_fullScreen) // Switching to fullscreen.
		{
			// Store the current window dimensions so they can be restored 
			// when switching out of fullscreen state.
			::GetWindowRect(m_hWnd, &m_WindowRect);
			// Set the window style to a borderless window so the client area fills
			// the entire screen.
			UINT windowStyle = WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);

			::SetWindowLong(m_hWnd, GWL_STYLE, windowStyle);

			// Query the name of the nearest display device for the window.
			// This is required to set the fullscreen dimensions of the window
			// when using a multi-monitor setup.
			HMONITOR hMonitor = ::MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
			MONITORINFOEX monitorInfo = {};
			monitorInfo.cbSize = sizeof(MONITORINFOEX);
			::GetMonitorInfo(hMonitor, &monitorInfo);
			::SetWindowPos(m_hWnd, HWND_TOP,
				monitorInfo.rcMonitor.left,
				monitorInfo.rcMonitor.top,
				monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
				monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
				SWP_FRAMECHANGED | SWP_NOACTIVATE);

			::ShowWindow(m_hWnd, SW_MAXIMIZE);
		}
		else
		{
			// Restore all the window decorators.
			::SetWindowLong(m_hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);

			::SetWindowPos(m_hWnd, HWND_NOTOPMOST,
				m_WindowRect.left,
				m_WindowRect.top,
				m_WindowRect.right - m_WindowRect.left,
				m_WindowRect.bottom - m_WindowRect.top,
				SWP_FRAMECHANGED | SWP_NOACTIVATE);

			::ShowWindow(m_hWnd, SW_NORMAL);
		}
	}
}
