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

#ifndef __UTILS_STRING__
#define __UTILS_STRING__

#include "ghost/stdint.h"
#include <stddef.h>

void stringConcat(const char* a, const char* b, char* out);

int stringCopy(char* target, const char* source);

int stringLength(const char* str);

int stringIndexOf(const char* str, char c);

bool stringEquals(const char* stra, const char* strb);

bool stringEquals(const char* straStart, const char* straEnd, const char* strbStart, const char* strbEnd);

bool stringEquals(const char* straStart, const char* straEnd, const char* strb);

void stringReplace(char* str, char character, char replacement);

int stringHash(const char* str);

char* stringDuplicate(const char* str);

char* stringWriteNumber(char* buffer, int number);

#endif
