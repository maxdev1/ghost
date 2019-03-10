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

#include "kernel/utils/stringbuffer.hpp"
#include "kernel/memory/memory.hpp"

struct g_stringbuffer {
	char* content;
	uint16_t size;
	uint16_t capacity;
};

g_stringbuffer* stringbufferCreate(uint16_t initialCapacity) {
	g_stringbuffer* buffer = (g_stringbuffer*) heapAllocate(sizeof(struct g_stringbuffer));
	buffer->capacity = initialCapacity;
	buffer->content = (char*) heapAllocate(sizeof(char) * buffer->capacity);
	buffer->size = 0;
	return buffer;
}

void stringbufferExtend(struct g_stringbuffer* buffer) {
	uint16_t oldCapacity = buffer->capacity;
	buffer->capacity += 16;

	char* newBuffer = (char*) heapAllocate(sizeof(char) * buffer->capacity);
	memoryCopy(newBuffer, buffer->content, oldCapacity);
	heapFree(buffer->content);
	buffer->content = newBuffer;
}

void stringbufferAppend(struct g_stringbuffer* buffer, char c) {
	buffer->content[buffer->size++] = c;

	if(buffer->size >= buffer->capacity) {
		stringbufferExtend(buffer);
	}
}

void stringbufferAppend(struct g_stringbuffer* buffer, const char* str) {
	while(*str) {
		stringbufferAppend(buffer, *str++);
	}
}

char* stringbufferGet(struct g_stringbuffer* buffer) {
	return buffer->content;
}

char* stringbufferTake(struct g_stringbuffer* buffer) {
	char* b = buffer->content;
	heapFree(buffer);
	return b;
}

void stringbufferRelease(struct g_stringbuffer* buffer) {
	heapFree(buffer->content);
	heapFree(buffer);
}
