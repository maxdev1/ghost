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

#include <ghost.h>
#include <ghostuser/io/files/file_utils.hpp>
#include <ghostuser/utils/logger.hpp>
#include <stdio.h>

/**
 *
 */
bool g_file_utils::read_string(g_fd fd, std::string& out) {

	std::stringstream s;

	uint8_t c;
	while (g_read(fd, &c, 1) > 0) {
		// stop on null-terminator
		if (c == 0) {
			out = s.str();
			return true;
		}

		s << (char) c;
	}

	// must have a null-terminator
	return false;
}

/**
 *
 */
bool g_file_utils::read_bytes(g_fd fd, uint8_t* buffer, size_t len) {

	ssize_t remain = len;

	g_fs_read_status status;
	while (remain) {
		ssize_t read = g_read_s(fd, &buffer[len - remain], remain, &status);
		if (status != G_FS_READ_SUCCESSFUL || read <= 0) {
			return false;
		}
		remain -= read;
	}

	return true;
}

/**
 *
 */
bool g_file_utils::read_bytes(g_fd fd, size_t offset, uint8_t* buffer, size_t len) {

	g_fs_seek_status s;
	g_seek_s(fd, offset, G_FS_SEEK_SET, &s);

	if (s != G_FS_SEEK_SUCCESSFUL) {
		return false;
	}

	return read_bytes(fd, buffer, len);
}

/**
 *
 */
bool g_file_utils::tryReadBytes(FILE* file, uint32_t offset, uint8_t* buffer, uint32_t len) {

	uint32_t remain = len;
	fseek(file, offset, SEEK_SET);

	while (remain) {
		size_t read = fread(&buffer[len - remain], 1, remain, file);
		if (read == 0) {
			return false;
		}
		remain -= read;
	}

	return true;
}
