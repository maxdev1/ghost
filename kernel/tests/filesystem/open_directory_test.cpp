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

#include "support/fixture.hpp"
#define GHOST_API_STDINT
#include <ghost/filesystem/types.h>

namespace
{
struct g_task {};
struct g_fs_node { unsigned id = 7; } root, cwd, directory;
using g_directory_iterator = g_fs_directory_iterator;
struct g_filesystem_find_result { g_fs_open_status status; g_fs_node* node; };
struct g_syscall_fs_open_directory
{
	const char* path;
	g_directory_iterator* iterator;
	g_fs_open_directory_status status;
};
static g_fs_node* seenOrigin;
static g_fs_open_status originStatus;
g_fs_open_status filesystemChooseOrigin(const char* path, g_task*, g_fs_node*& origin)
{
	origin = path[0] == '/' ? &root : &cwd;
	return originStatus;
}
g_filesystem_find_result filesystemFind(g_fs_node* origin, const char*)
{
	seenOrigin = origin;
	return {G_FS_OPEN_SUCCESSFUL, &directory};
}
g_fs_open_directory_status filesystemOpenDirectory(g_fs_node*) { return G_FS_OPEN_DIRECTORY_SUCCESSFUL; }

bool memoryCopyUserString(g_task*, const char* source, char* target, size_t capacity) {
    if(!source || strlen(source) >= capacity) return false;
    strcpy(target, source); return true;
}
bool memoryCopyFromUser(g_task*, void* target, g_address source, size_t length) {
    memcpy(target, (void*) source, length); return true;
}
bool memoryCopyToUser(g_task*, g_address target, const void* source, size_t length) {
    memcpy((void*) target, source, length); return true;
}
#include "support/memory_user.hpp"
#include "production/filesystem_open_directory.inc"

TEST(OpenDirectory, ResolvesRelativePathsFromTheCallingTasksWorkingDirectory)
{
	g_task task;
	g_directory_iterator iterator{99, 99};
	originStatus = G_FS_OPEN_SUCCESSFUL;
	for(auto path: {"child", "../child", "/child"})
	{
		g_syscall_fs_open_directory call{path, &iterator, G_FS_OPEN_DIRECTORY_ERROR};
		syscallFsOpenDirectory(&task, &call);
		EXPECT_EQ(path[0] == '/' ? &root : &cwd, seenOrigin);
		EXPECT_EQ(G_FS_OPEN_DIRECTORY_SUCCESSFUL, call.status);
		EXPECT_EQ(directory.id, iterator.node_id);
		EXPECT_EQ(0u, iterator.position);
	}
}

TEST(OpenDirectory, ReportsAnUnresolvableWorkingDirectoryWithoutLookingUpThePath)
{
	g_task task;
	g_directory_iterator iterator{99, 99};
	g_syscall_fs_open_directory call{"child", &iterator, G_FS_OPEN_DIRECTORY_SUCCESSFUL};
	seenOrigin = nullptr;
	originStatus = G_FS_OPEN_NOT_FOUND;
	syscallFsOpenDirectory(&task, &call);
	EXPECT_EQ(G_FS_OPEN_DIRECTORY_NOT_FOUND, call.status);
	EXPECT_EQ(nullptr, seenOrigin);
	EXPECT_EQ(99u, iterator.position);
}
}
