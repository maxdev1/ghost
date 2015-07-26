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

#ifndef GHOST_API_CALLS_MISCCALLS
#define GHOST_API_CALLS_MISCCALLS

/**
 * @field message
 * 		the message
 */
typedef struct {
	char* message;
}__attribute__((packed)) g_syscall_log;

/**
 * @field enabled
 * 		whether or not to enable the video log
 */
typedef struct {
	uint8_t enabled;
}__attribute__((packed)) g_syscall_set_video_log;

/**
 * @field test
 * 		test value
 *
 * @field result
 * 		test result
 */
typedef struct {
	uint32_t test;

	uint32_t result;
}__attribute__((packed)) g_syscall_test;

#endif
