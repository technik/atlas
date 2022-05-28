#pragma once

#include <Windows.h>
#include <cstdint>
#include <DirectXMath.h>

class Window
{
public:
	static Window* CreateNewWindow(const char* windowName, uint32_t width, uint32_t height);

	void setFullScreen(bool fullscreen);
	bool isFullScreen() const { return m_fullScreen; }

	// Window handle.
	HWND m_hWnd = NULL;
	// Window rectangle (used to toggle fullscreen state).
	RECT m_WindowRect;
private:
	Window() = default;

	bool m_fullScreen = false;
};