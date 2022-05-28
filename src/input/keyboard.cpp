#include "keyboard.h"

#include <cassert>

namespace input
{
	//------------------------------------------------------------------------------------------------------------------
	void Keyboard::init()
	{
		assert(0 == sInstance);
		sInstance = new Keyboard();
	}

	//------------------------------------------------------------------------------------------------------------------
	void Keyboard::end()
	{
		assert(0 != sInstance);
		delete sInstance;
		sInstance = 0;
	}

	//------------------------------------------------------------------------------------------------------------------
	Keyboard::Keyboard()
	{
		for (int key = 0; key < MaxKeys; ++key)
		{
			keyState[key] = 0;
			oldKeyState[key] = 0;
		}
	}

	//------------------------------------------------------------------------------------------------------------------
	bool Keyboard::pressed(uint8_t _key) const
	{
		return keyState[(unsigned)_key] == 1 && oldKeyState[(unsigned)_key] == 0;
	}

	//------------------------------------------------------------------------------------------------------------------
	bool Keyboard::held(uint8_t _key) const
	{
		return keyState[(unsigned)_key] == 1;
	}

	//------------------------------------------------------------------------------------------------------------------
	bool Keyboard::released(uint8_t _key) const
	{
		return keyState[(unsigned)_key] == 0;
	}

	//------------------------------------------------------------------------------------------------------------------
	void Keyboard::refresh()
	{
		// Save old state
		for (int key = 0; key < MaxKeys; ++key)
		{
			oldKeyState[key] = keyState[key];
		}
	}

	//------------------------------------------------------------------------------------------------------------------
	bool Keyboard::processWin32Message(MSG msg) {
		if (msg.message == WM_KEYDOWN)
		{
			if (msg.wParam < MaxKeys)
			{
				keyState[msg.wParam] = 1;
				return true;
			}
		}
		else if (msg.message == WM_KEYUP)
		{
			if (msg.wParam < MaxKeys)
			{
				keyState[msg.wParam] = 0;
				return true;
			}
		}
		return false;
	}

}	// namespace input