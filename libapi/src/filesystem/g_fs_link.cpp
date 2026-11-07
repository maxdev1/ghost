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

#include "ghost/filesystem/link.h"

int g_fs_link_name(const char* name)
{
	const char* end = name;
	while(*end)
		++end;
	if(end - name < 5)
		return 0;
	end -= 5;
	return end[0] == '.' && //
		   (end[1] == 'l' || end[1] == 'L') && // l
		   (end[2] == 'i' || end[2] == 'I') && // o
		   (end[3] == 'n' || end[3] == 'N') && // l
		   (end[4] == 'k' || end[4] == 'K'); // kernel has no tolower
}

g_fs_node_type g_fs_storage_node_type(g_fs_node_type type, const char* name)
{
	if(type == G_FS_NODE_TYPE_FILE || type == G_FS_NODE_TYPE_SYMLINK)
		return g_fs_link_name(name) ? G_FS_NODE_TYPE_SYMLINK : G_FS_NODE_TYPE_FILE;

	return type;
}

int g_fs_link_parse(const char* bytes, unsigned length, char* target)
{
	const char prefix[] = "target=";
	if(length < G_FS_LINK_PREFIX_LENGTH || length > G_FS_LINK_STORAGE_MAX)
		return -1;

	for(unsigned i = 0; i < G_FS_LINK_PREFIX_LENGTH; ++i)
		if(bytes[i] != prefix[i])
			return -1;

	if(length && bytes[length - 1] == '\n')
	{
		--length;
		if(length && bytes[length - 1] == '\r')
			--length;
	}
	unsigned count = length - G_FS_LINK_PREFIX_LENGTH;
	if(!count || count >= G_PATH_MAX)
		return -1;
	for(unsigned i = 0; i < count; ++i)
	{
		unsigned char value = bytes[G_FS_LINK_PREFIX_LENGTH + i];
		if(value < 32 || value == 127)
			return -1;
		if(target)
			target[i] = value;
	}
	if(target)
		target[count] = 0;
	return count;
}
