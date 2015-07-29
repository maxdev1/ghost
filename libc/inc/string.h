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

#ifndef __GHOST_LIBC_STRING__
#define __GHOST_LIBC_STRING__

#include "ghost/common.h"
#include <stddef.h>

__BEGIN_C

/**
 * Copies <num> bytes from <src> to <dest>.
 *
 * @param dest
 * 		destination memory location
 * @param src
 * 		source memory location
 * @param num
 * 		number of bytes to copy
 * @return
 * 		<dest> is returned
 */
void* memcpy(void* dest, const void* src, size_t num);

/**
 * Copies <num> bytes from <src> to <dest>, allowing the
 * memory areas of source and destination to overlap.
 *
 * @param dest
 * 		destination memory location
 * @param src
 * 		source memory location
 * @param num
 * 		number of bytes to copy
 * @return
 * 		<dest> is returned
 */
void* memmove(void* dest, const void* src, size_t num);

void* memset(void* mem, int value, size_t len);
int memcmp(const void* mem_a, const void* mem_b, size_t len);
void* memchr(const void* mem, int value, size_t num);

char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, size_t max);

char* strcat(char* dest, const char* src);
char* strncat(char* dest, const char* src, size_t max);

int strcmp(const char* str_a, const char* str_b);
int strncmp(const char* str_a, const char* str_b, size_t max);

int strcoll(const char* str_a, const char* str_b);
size_t strxfrm(char* dest, const char* src, size_t num);

char* strchr(const char* str, int value);
char* strrchr(const char* str, int value);

size_t strcspn(const char* str_a, const char* str_b);
size_t strspn(const char* str_a, const char* str_b);
char* strpbrk(const char* str_a, const char* str_b);
char* strstr(const char* str_a, const char* str_b);
char* strtok(char* dest, const char* src);

size_t strlen(const char* str);

char* strdup(const char *s);

char* strerror(int errno);

__END_C

#endif
