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

#ifndef GHOST_FILESYSTEM_LINK
#define GHOST_FILESYSTEM_LINK

#include "types.h"

#define G_FS_LINK_LIMIT 40
#define G_FS_LINK_PREFIX_LENGTH 7
#define G_FS_LINK_STORAGE_MAX (G_PATH_MAX + G_FS_LINK_PREFIX_LENGTH + 1)

__BEGIN_C

/**
 * The reserved suffix is ASCII case-insensitive on every filesystem. Directories
 * are unaffected; links are stored using ordinary regular-file metadata.
 */
int g_fs_link_name(const char* name);

g_fs_node_type g_fs_storage_node_type(g_fs_node_type type, const char* name);

/**
 * Parses exactly one target= line, with an optional LF or CRLF terminator.
 * Spaces and UTF-8 bytes are preserved. Empty targets, NUL/control bytes, extra
 * lines and targets longer than G_PATH_MAX-1 bytes are rejected.
 */
int g_fs_link_parse(const char* bytes, unsigned length, char* target);

__END_C

#endif
