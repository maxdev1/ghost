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

#ifndef __UTILS_STRING_BUFFER__
#define __UTILS_STRING_BUFFER__

#include "ghost/types.h"

/**
 * String buffer type
 */
struct g_stringbuffer;

/**
 * Creates a new string buffer with a given initial capacity.
 * 
 * @param initialCapacity the initially reserved capacity
 * @return buffer instance
 */
g_stringbuffer* stringbufferCreate(uint16_t initialCapacity);

/**
 * Appends a character to the buffer.
 * 
 * @param buffer instance
 * @param c the character
 */
void stringbufferAppend(g_stringbuffer* buffer, char c);

/**
 * Appends a string to the buffer.
 * 
 * @param buffer instance
 * @param str the string
 */
void stringbufferAppend(struct g_stringbuffer* buffer, const char* str);

/**
 * Returns the content buffer.
 * 
 * @param buffer instance
 * @return the content buffer
 */
char* stringbufferGet(g_stringbuffer* buffer);

/**
 * Returns the content buffer and releases other allocated memory.
 * 
 * @param buffer instance
 * @return the content buffer
 */
char* stringbufferTake(g_stringbuffer* buffer);

/**
 * Releases all allocated memory including the content buffer.
 * 
 * @param buffer
 */
void stringbufferRelease(g_stringbuffer* buffer);

#endif