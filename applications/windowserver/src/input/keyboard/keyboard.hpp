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

#ifndef __WINDOWSERVER_INPUT_KEYBOARD__
#define __WINDOWSERVER_INPUT_KEYBOARD__

#include <sstream>
#include <stdint.h>
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

    bool operator<(const g_key_info& other) const
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

class g_keyboard
{
  private:
    static void registerKeyboard();

  public:
    static g_key_info readKey(g_fd in);

    static bool keyForScancode(uint8_t scancode, g_key_info* out);
    static char charForKey(g_key_info info);
    static g_key_info fullKeyInfo(g_key_info_basic basic);

    static std::string getCurrentLayout();
    static bool loadLayout(std::string iso);
    static bool loadScancodeLayout(std::string iso);
    static bool loadConversionLayout(std::string iso);
};

#endif
