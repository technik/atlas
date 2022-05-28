//----------------------------------------------------------------------------------------------------------------------
// Revolution Engine
//----------------------------------------------------------------------------------------------------------------------
#include "mouse.h"

#include <cassert>

namespace input
{
	//------------------------------------------------------------------------------------------------------------------
	void Mouse::init()
	{
		assert(!sInstance);
		sInstance = new Mouse();
	}

	//------------------------------------------------------------------------------------------------------------------
	bool Mouse::processMessage(MSG message)
	{
		if (message.message == WM_LBUTTONDOWN)
		{
			mLeftDown = true;
			return true;
		}
		if (message.message == WM_LBUTTONUP)
		{
			mLeftDown = false;
			return true;
		}
		if (message.message == WM_RBUTTONDOWN)
		{
			mRightDown = true;
			return true;
		}
		if (message.message == WM_RBUTTONUP)
		{
			mRightDown = false;
			return true;
		}
		if (message.message == WM_MBUTTONDOWN)
		{
			mMiddleDown = true;
			return true;
		}
		if (message.message == WM_MBUTTONUP)
		{
			mMiddleDown = false;
			return true;
		}
		if (message.message == WM_MOUSEMOVE || message.message == WM_MOUSEHOVER)
		{
			auto pos = MAKEPOINTS(message.lParam);
			m_position.x() = pos.x;
			m_position.y() = pos.y;
			return true;
		}
		return false; // Nothing processed
	}

} // namespace input
