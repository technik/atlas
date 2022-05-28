//----------------------------------------------------------------------------------------------------------------------
// Revolution Engine
//----------------------------------------------------------------------------------------------------------------------
#pragma once

#include <Windows.h>
#include <math/vector.h>

namespace input
{
	class Mouse
	{
	public:
		// Singleton interface
		static void init();
		static Mouse* get() { return sInstance; }

		math::Vec2i position() const { return m_position; }
		bool		leftDown() const { return mLeftDown; }
		bool		middleDown() const { return mMiddleDown; }
		bool		rightDown() const { return mRightDown; }

		// Windows specific interface
		bool processMessage(MSG message);

	private:
		math::Vec2i m_position;
		bool		mLeftDown = false;
		bool		mMiddleDown = false;
		bool		mRightDown = false;

		inline static Mouse* sInstance = nullptr;
	};
} // namespace input
