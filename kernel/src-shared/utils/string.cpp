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

#include <memory/memory.hpp>
#include <utils/string.hpp>

/**
 *
 */
void g_string::concat(const char* a, const char* b, char* out) {
	int len_a = g_string::length(a);
	int len_b = g_string::length(b);
	g_memory::copy(out, a, len_a);
	g_memory::copy(&out[len_a], b, len_b);
	out[len_a + len_b] = 0;
}

/**
 *
 */
void g_string::copy(char* target, const char* source) {
	int len = length(source);
	g_memory::copy(target, source, len);
	target[len] = 0;
}

/**
 *
 */
int g_string::length(const char* str) {
	int size = 0;
	while (*str++) {
		++size;
	}
	return size;
}

/**
 *
 */
int g_string::indexOf(const char* str, char c) {
	int pos = 0;
	while (*str) {
		if (*str == c) {
			return pos;
		}
		++pos;
		++str;
	}
	return -1;
}

/**
 * Returns if the given cstring equals this string
 */
bool g_string::equals(const char* stra, const char* strb) {

	if (stra == strb)
		return true;

	int alen = length(stra);
	int blen = length(strb);

	if (alen != blen) {
		return false;
	}

	while (alen-- > 0) {
		if (stra[alen] != strb[alen]) {
			return false;
		}
	}
	return true;
}

/**
 * Replaces all occurences of char a with char b in the given cstring
 */
void g_string::replace(char* str, char character, char replacement) {
	for (uint32_t i = 0;; i++) {
		if (str[i] == 0) {
			break;
		}

		if (str[i] == character) {
			str[i] = replacement;
		}
	}
}

