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

#include <ghostuser/utils/utils.hpp>
#include <cwctype>

/**
 *
 */
std::string g_utils::trim(std::string str) {
	int left = 0;
	while (isspace(str[left])) {
		++left;
	}

	int right = str.length();
	while (isspace(str[right - 1])) {
		--right;
	}

	if (right <= left) {
		return "";
	}

	return str.substr(left, right - left);
}

/**
 * Reads a byte from the given CPU port
 */
uint8_t g_utils::inportByte(uint16_t port) {
	uint8_t value;
	asm volatile("inb %1, %0" : "=a" (value) : "dN" (port));
	return value;
}

/**
 * Writes the given data to the given CPU port
 */
void g_utils::outportByte(uint16_t port, uint8_t value) {
	asm volatile("outb %1, %0" : : "dN" (port), "a" (value));
}

/**
 * Reads a short from the given CPU port
 */
uint16_t g_utils::inportShort(uint16_t port) {
	uint16_t value;
	asm volatile("inw %1, %0" : "=a" (value) : "dN" (port));
	return value;
}

/**
 * Writes the given data to the given CPU port
 */
void g_utils::outportShort(uint16_t port, uint16_t value) {
	asm volatile("outw %1, %0" : : "dN" (port), "a" (value));
}

/**
 * Reads a dword from the given CPU port
 */
uint32_t g_utils::inportInt(uint16_t port) {
	uint32_t value;
	asm volatile("inl %1, %0" : "=a" (value) : "dN" (port));
	return value;
}

/**
 * Writes the given data to the given CPU port
 */
void g_utils::outportInt(uint16_t port, uint32_t value) {
	asm volatile("outl %1, %0" : : "dN" (port), "a" (value));
}
