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

#include "ghost/user.h"

/**
 *
 */
g_get_working_directory_status g_get_working_directory(char* buffer) {
	return g_get_working_directory_l(buffer, G_PATH_MAX);
}
/**
 *
 */
g_get_working_directory_status g_get_working_directory_l(char* buffer, size_t maxlen) {
	g_syscall_fs_get_working_directory data;
	data.buffer = buffer;
	data.maxlen = maxlen;
	g_syscall(G_SYSCALL_GET_WORKING_DIRECTORY, (uint32_t) &data);
	return data.result;
}
