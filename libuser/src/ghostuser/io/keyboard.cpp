/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2015, Max Schlüssel <lokoxe@gmail.com>                     *
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

#include <ghost.h>
#include <ghostuser/io/files/file_utils.hpp>
#include <ghostuser/io/keyboard.hpp>
#include <ghostuser/io/ps2.hpp>
#include <ghostuser/io/ps2_driver_constants.hpp>
#include <ghostuser/tasking/ipc.hpp>
#include <ghostuser/utils/logger.hpp>
#include <ghostuser/utils/property_file_parser.hpp>
#include <ghostuser/utils/utils.hpp>
#include <string>
#include <map>
#include <fstream>
#include <stdio.h>

static bool statusCtrl = false;
static bool statusShift = false;
static bool statusAlt = false;

// TODO
static bool numLock = false;

static std::map<uint8_t, std::string> scancodeLayout;
static std::map<g_key_info, char> conversionLayout;

static uint32_t keyboardRegisteredTask = -1;
static uint32_t keyboardTopic = -1;

static std::string currentLayout;

/**
 *
 */
g_key_info g_keyboard::readKey(bool* break_condition) {

	if (!g_ps2_is_registered) {
		if (!g_ps2::registerSelf()) {
			return g_key_info();
		}
	}

	// wait until incoming data is here (and the driver unsets the atom)
	g_atomic_block_2(&g_ps2_area->keyboard.atom_nothing_queued, (uint8_t*) break_condition);

	// take info from the shared memory
	uint8_t scancode = g_ps2_area->keyboard.scancode;

	// tell driver that we've handled it
	g_ps2_area->keyboard.atom_nothing_queued = true;
	g_ps2_area->keyboard.atom_unhandled = false;

	// read and convert data
	return keyForScancode(scancode);
}

/**
 *
 */
g_key_info g_keyboard::keyForScancode(uint8_t scancode) {

	g_key_info info;

	// Get "pressed" info from scancode
	info.pressed = !(scancode & (1 << 7));
	scancode = scancode & ~(1 << 7); // remove 7th bit

	info.scancode = scancode;

	// Get key from layout map
	auto pos = scancodeLayout.find(scancode);
	if (pos != scancodeLayout.end()) {
		info.key = pos->second;
	}

	// Handle special keys
	if (info.key == "KEY_CTRL_L" || info.key == "KEY_CTRL_R") {
		statusCtrl = info.pressed;

	} else if (info.key == "KEY_SHIFT_L" || info.key == "KEY_SHIFT_R") {
		statusShift = info.pressed;

	} else if (info.key == "KEY_ALT_L" || info.key == "KEY_ALT_R") {
		statusAlt = info.pressed;

	} else if (info.key == "KEY_PAD_8") {
		info.key = "KEY_ARROW_UP";
	} else if (info.key == "KEY_PAD_6") {
		info.key = "KEY_ARROW_RIGHT";
	} else if (info.key == "KEY_PAD_4") {
		info.key = "KEY_ARROW_LEFT";
	} else if (info.key == "KEY_PAD_2") {
		info.key = "KEY_ARROW_DOWN";
	}

	// Set control key info
	info.ctrl = statusCtrl;
	info.shift = statusShift;
	info.alt = statusAlt;

	return info;
}

/**
 *
 */
char g_keyboard::charForKey(g_key_info info) {

	auto pos = conversionLayout.find(info);
	if (pos != conversionLayout.end()) {
		return pos->second;
	}

	return -1;
}

/**
 *
 */
bool g_keyboard::loadLayout(std::string iso) {
	if (loadScancodeLayout(iso) && loadConversionLayout(iso)) {
		currentLayout = iso;
		return true;
	}
	return false;
}

/**
 *
 */
std::string g_keyboard::getCurrentLayout() {
	return currentLayout;
}

/**
 *
 */
bool g_keyboard::loadScancodeLayout(std::string iso) {

	// Clear layout and parse file
	std::ifstream in("/system/keyboard/" + iso + ".layout");
	if (!in.good()) {
		return false;
	}
	g_property_file_parser props(in);

	scancodeLayout.clear();
	std::map<std::string, std::string> properties = props.getProperties();

	for (auto entry : properties) {
		// Convert number from hex
		std::stringstream scancodeStr;
		scancodeStr << std::hex << entry.first;

		uint32_t scancode;
		scancodeStr >> scancode;

		if (entry.second.empty()) {
			std::stringstream msg;
			msg << "could not map scancode ";
			msg << (uint32_t) scancode;
			msg << ", key name '";
			msg << entry.second;
			msg << "' is not known";
			g_logger::log(msg.str());
		} else {
			scancodeLayout[scancode] = entry.second;
		}
	}

	return true;
}

/**
 *
 */
bool g_keyboard::loadConversionLayout(std::string iso) {

	// Read layout file
	std::stringstream conversionLayoutStream;

	// Clear layout and parse file
	std::ifstream in("/system/keyboard/" + iso + ".conversion");
	if (!in.good()) {
		return false;
	}
	g_property_file_parser props(in);

	// empty existing conversion layout
	conversionLayout.clear();

	std::map<std::string, std::string> properties = props.getProperties();
	for (auto entry : properties) {

		// create key info value
		g_key_info info;

		// they shall be triggered on press
		info.pressed = true;

		// take the key and split if necessary
		std::string keyName = entry.first;
		int spacePos = keyName.find(' ');

		if (spacePos != -1) {
			std::string flags = keyName.substr(spacePos + 1);
			keyName = keyName.substr(0, spacePos);

			// Handle the flags
			for (int i = 0; i < flags.length(); i++) {

				if (flags[i] == 's') {
					info.shift = true;

				} else if (flags[i] == 'c') {
					info.ctrl = true;

				} else if (flags[i] == 'a') {
					info.alt = true;

				} else {
					std::stringstream msg;
					msg << "unknown flag in conversion mapping: ";
					msg << flags[i];
					g_logger::log(msg.str());
				}
			}
		}

		// Set key
		info.key = keyName;

		// Push the mapping
		char c = -1;
		std::string value = entry.second;
		if (value.length() > 0) {
			c = value[0];

			// Escaped numeric values
			if (c == '\\') {
				if (value.length() > 1) {
					std::stringstream conv;
					conv << value.substr(1);
					uint32_t num;
					conv >> num;
					c = num;

				} else {
					g_logger::log("skipping value '" + value + "' in key " + keyName + ", illegal format");
					continue;
				}
			}
		}
		conversionLayout[info] = c;
	}

	return true;
}

