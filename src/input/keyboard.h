
#pragma once

#include <Windows.h>
#include <cstdint>

namespace input
{
	class Keyboard
	{
	public:

		enum class Key : uint8_t
		{
			BackSpace = VK_BACK,
			Tab = VK_TAB,
			Escape = VK_ESCAPE,
			Enter = VK_RETURN,
			Shift = VK_SHIFT,
			Control,
			Alt,
			Pause,
			CapsLock,
			PageUp = VK_PRIOR,
			PageDown,
			End,
			Home,
			KeyLeft,
			KeyUp,
			KeyRight,
			KeyDown,
			Delete = VK_DELETE,
			Key0 = VK_NUMPAD0,
			Key1, Key2, Key3, Key4, Key5, Key6, Key7, Key8, Key9, Key,
		};

		Keyboard();

		static void				init();
		static void				end();
		static Keyboard* get() { return sInstance; }

		bool pressed(Key _key) const { return pressed(uint8_t(_key)); }
		bool held(Key _key) const { return held(uint8_t(_key)); }
		bool released(Key _key) const { return released(uint8_t(_key)); }

		bool pressed(uint8_t _key) const;
		bool held(uint8_t _key) const;
		bool released(uint8_t _key) const;

		void refresh();
		bool processWin32Message(MSG);

	private:
		static constexpr uint32_t MaxKeys = 512;
		int keyState[MaxKeys];
		int oldKeyState[MaxKeys];

	private:
		inline static Keyboard* sInstance = nullptr;
	};
}	// namespace input