#ifndef __LIBWINDOW_PLATFORM_KEY_INFO__
#define __LIBWINDOW_PLATFORM_KEY_INFO__

/**
 * Ghost-specific type definitions
*/
#ifdef _GHOST_

#include <libinput/keyboard/keyboard.hpp>

/**
 * Windows-MinGW-specific type definitions
 */
#elif _WIN32

#include <string>

struct g_key_info_basic
{
	bool pressed : 1;
	bool ctrl : 1;
	bool alt : 1;
	bool shift : 1;
	uint16_t scancode;

	g_key_info_basic() : pressed(false), ctrl(false), alt(false), shift(false), scancode(0)
	{
	}
} __attribute__((packed));

class g_key_info : public g_key_info_basic
{
public:
	g_key_info() : key("KEY_NONE")
	{
	}

	std::string key;

	bool operator<(const g_key_info &other) const
	{
		if(key > other.key)
			return false;
		if(key < other.key)
			return true;

		if(pressed > other.pressed)
			return false;
		if(pressed < other.pressed)
			return true;

		if(ctrl > other.ctrl)
			return false;
		if(ctrl < other.ctrl)
			return true;

		if(alt > other.alt)
			return false;
		if(alt < other.alt)
			return true;

		if(shift > other.shift)
			return false;
		if(shift < other.shift)
			return true;

		return false;
	}
};


#endif


#endif
