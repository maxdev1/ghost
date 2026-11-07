/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2026, Max Schlüssel <lokoxe@gmail.com>                     *
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

#ifndef GHOST_BOOT_FAT_ROOT
#define GHOST_BOOT_FAT_ROOT
#include <stdint.h>

inline bool kernelFatRootParseHex(const char* text, uint32_t* value)
{
	*value = 0;
	for(unsigned i = 0; i < 8; ++i)
	{
		auto digit = text[i];
		if(digit >= '0' && digit <= '9')
			*value = (*value << 4) | (digit - '0');
		else if(digit >= 'a' && digit <= 'f')
			*value = (*value << 4) | (digit - 'a' + 10);
		else
			return false;
	}

	return true;
}

inline bool kernelFatRootValidateSelector(const char* command, unsigned length)
{
	if(length != 17 && length != 26)
		return false;

	const char* prefix = "root=fat:";
	for(unsigned i = 0; i < 9; ++i)
		if(command[i] != prefix[i])
			return false;

	uint32_t disk;
	if(!kernelFatRootParseHex(command + 9, &disk) || !disk)
		return false;
	if(length == 17)
		return true;

	uint32_t partition;
	return command[17] == ':' && kernelFatRootParseHex(command + 18, &partition);
}

/**
 * Converts the boot disk identity and optional partition LBA into the argument
 * passed through device manager to the FAT driver. Legacy disk selectors work.
 */
inline bool kernelFatRootArguments(const char* command, char (&arguments)[32])
{
	if(!command)
		return false;

	unsigned length = 0;
	while(length <= 26 && command[length])
		++length;

	if(!kernelFatRootValidateSelector(command, length))
		return false;

	for(auto& c: arguments)
		c = 0;

	const char* option = "--root-disk\x1f";
	for(unsigned i = 0; i < 12; ++i)
		arguments[i] = option[i];
	for(unsigned i = 9; i < length; ++i)
		arguments[12 + i - 9] = command[i];
	return true;
}
#endif
