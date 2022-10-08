/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2022, Max Schlüssel <lokoxe@gmail.com>                     *
 *                                                                           *
 *  This program is free software: you can redistribute it and/or modify     *
 *  it under the terms of the GNU General Public License as published by     *
 *  the Free Software Foundation, either version 3 of the License, or        *
 *  (at your option) any later version.                                      *
 *                                                                           *
 *  This program is distributed in the hope that it will be useful,          *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *  GNU General Public License for more details.                             *
 *                                                                           *
 *  You should have received a copy of the GNU General Public License        *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
 *                                                                           *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "libinput/keyboard/keyboard.hpp"

#include <fstream>
#include <ghost.h>
#include <libproperties/parser.hpp>
#include <libps2driver/ps2driver.hpp>
#include <map>
#include <stdio.h>
#include <string>

static bool statusCtrl = false;
static bool statusShift = false;
static bool statusAlt = false;

// TODO
static std::map<uint32_t, std::string> scancodeLayout;
static std::map<g_key_info, char> conversionLayout;

static std::string currentLayout;

static g_key_info last_unknown_key;
static bool have_last_unknown_key = false;

g_key_info g_keyboard::readKey(g_fd in)
{
	uint8_t scancode[1];
	if(g_read(in, scancode, 1) > 0)
	{
		g_key_info info;
		if(keyForScancode(scancode[0], &info))
		{
			return info;
		}
	}

	return g_key_info();
}

bool g_keyboard::keyForScancode(uint8_t scancode, g_key_info *out)
{

	// Get "pressed" info from scancode
	out->pressed = !(scancode & (1 << 7));
	out->scancode = scancode & ~(1 << 7); // remove 7th bit

	// Get key from layout map
	bool found_compound = false;
	if(have_last_unknown_key)
	{
		int compoundScancode = last_unknown_key.scancode << 8 | out->scancode;

		// Try to find a compound key
		auto pos = scancodeLayout.find(compoundScancode);
		if(pos != scancodeLayout.end())
		{
			out->key = pos->second;
			out->scancode = compoundScancode;
			found_compound = true;
			have_last_unknown_key = false;
		}
	}

	// When it is no compound
	if(!found_compound)
	{

		// Try to find the normal key
		auto pos = scancodeLayout.find(out->scancode);
		if(pos == scancodeLayout.end())
		{

			// If it's not found, this might be the start of a compound
			have_last_unknown_key = true;
			last_unknown_key = *out;
			return false;
		}
		else
		{
			out->key = pos->second;
		}
	}

	// Handle special keys
	if(out->key == "KEY_CTRL_L" || out->key == "KEY_CTRL_R")
	{
		statusCtrl = out->pressed;
	}
	else if(out->key == "KEY_SHIFT_L" || out->key == "KEY_SHIFT_R")
	{
		statusShift = out->pressed;
	}
	else if(out->key == "KEY_ALT_L" || out->key == "KEY_ALT_R")
	{
		statusAlt = out->pressed;
	}

	// Set control key info
	out->ctrl = statusCtrl;
	out->shift = statusShift;
	out->alt = statusAlt;

	return true;
}

g_key_info g_keyboard::fullKeyInfo(g_key_info_basic basic)
{

	// Get key from layout map
	g_key_info info;
	info.alt = basic.alt;
	info.ctrl = basic.ctrl;
	info.pressed = basic.pressed;
	info.scancode = basic.scancode;
	info.shift = basic.shift;

	auto pos = scancodeLayout.find(basic.scancode);
	if(pos != scancodeLayout.end())
	{
		info.key = pos->second;
	}
	return info;
}

char g_keyboard::charForKey(g_key_info info)
{

	auto pos = conversionLayout.find(info);
	if(pos != conversionLayout.end())
	{
		return pos->second;
	}

	return -1;
}

bool g_keyboard::loadLayout(std::string iso)
{
	if(loadScancodeLayout(iso) && loadConversionLayout(iso))
	{
		currentLayout = iso;
		return true;
	}
	return false;
}

std::string g_keyboard::getCurrentLayout()
{
	return currentLayout;
}

bool g_keyboard::loadScancodeLayout(std::string iso)
{

	// Clear layout and parse file
	std::ifstream in("/system/keyboard/" + iso + ".layout");
	if(!in.good())
	{
		klog("scancode layout file '%s' not found", iso.c_str());
		return false;
	}
	g_properties_parser props(in);

	scancodeLayout.clear();
	std::map<std::string, std::string> properties = props.getProperties();

	for(auto entry : properties)
	{

		uint32_t scancode = 0;

		auto spacepos = entry.first.find(" ");
		if(spacepos != std::string::npos)
		{
			std::string part1 = entry.first.substr(0, spacepos);
			std::string part2 = entry.first.substr(spacepos + 1);

			uint32_t part1val;
			std::stringstream conv1;
			conv1 << std::hex << part1;
			conv1 >> part1val;

			uint32_t part2val;
			std::stringstream conv2;
			conv2 << std::hex << part2;
			conv2 >> part2val;

			scancode = (part1val << 8) | part2val;
		}
		else
		{
			std::stringstream conv;
			conv << std::hex << entry.first;
			conv >> scancode;
		}

		if(entry.second.empty())
		{
			std::stringstream msg;
			msg << "could not map scancode ";
			msg << (uint32_t) scancode;
			msg << ", key name '";
			msg << entry.second;
			msg << "' is not known";
			klog(msg.str().c_str());
		}
		else
		{
			scancodeLayout[scancode] = entry.second;
		}
	}

	return true;
}

bool g_keyboard::loadConversionLayout(std::string iso)
{

	// Read layout file
	std::stringstream conversionLayoutStream;

	// Clear layout and parse file
	std::ifstream in("/system/keyboard/" + iso + ".conversion");
	if(!in.good())
	{
		klog("conversion file for '%s' not found", iso.c_str());
		return false;
	}
	g_properties_parser props(in);

	// empty existing conversion layout
	conversionLayout.clear();

	std::map<std::string, std::string> properties = props.getProperties();
	for(auto entry : properties)
	{

		// create key info value
		g_key_info info;

		// they shall be triggered on press
		info.pressed = true;

		// take the key and split if necessary
		std::string keyName = entry.first;
		int spacePos = keyName.find(' ');

		if(spacePos != -1)
		{
			std::string flags = keyName.substr(spacePos + 1);
			keyName = keyName.substr(0, spacePos);

			// Handle the flags
			for(int i = 0; i < flags.length(); i++)
			{

				if(flags[i] == 's')
				{
					info.shift = true;
				}
				else if(flags[i] == 'c')
				{
					info.ctrl = true;
				}
				else if(flags[i] == 'a')
				{
					info.alt = true;
				}
				else
				{
					std::stringstream msg;
					msg << "unknown flag in conversion mapping: ";
					msg << flags[i];
					klog(msg.str().c_str());
				}
			}
		}

		// Set key
		info.key = keyName;

		// Push the mapping
		char c = -1;
		std::string value = entry.second;
		if(value.length() > 0)
		{
			c = value[0];

			// Escaped numeric values
			if(c == '\\')
			{
				if(value.length() > 1)
				{
					std::stringstream conv;
					conv << value.substr(1);
					uint32_t num;
					conv >> num;
					c = num;
				}
				else
				{
					klog("skipping value '%s' in key %s, illegal format", value, keyName);
					continue;
				}
			}
		}
		conversionLayout[info] = c;
	}

	return true;
}
