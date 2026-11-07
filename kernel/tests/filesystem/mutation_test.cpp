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
struct g_task {} task;
struct g_fs_node { g_fs_node_type type; } file{G_FS_NODE_TYPE_FILE};
struct g_filesystem_find_result
{
    g_fs_node* node;
    bool foundAllButLast;
    g_fs_node* lastFoundParent;
    const char* fileNameStart;
};

struct g_fs_delegate
{
    g_fs_mutation_status (*makeDirectory)(g_fs_node*, const char*, g_fs_node**);
    g_fs_mutation_status (*mutate)(g_fs_node*, g_fs_delegate_request_type, uint64_t);
} delegate;
static g_fs_open_status lookupStatus;
static unsigned mutations;
g_fs_open_status filesystemMutationFind(g_task*, const char*, g_filesystem_find_result* result)
{
    *result = {&file, true, &file, "child"};
    return lookupStatus;
}

g_fs_delegate* filesystemFindDelegate(g_fs_node*) { return &delegate; }
g_fs_mutation_status mutationCreate(g_fs_node*, const char*, g_fs_node**)
{
    ++mutations;
    return G_FS_MUTATION_NO_SPACE;
}

g_fs_mutation_status mutationRemove(g_fs_node*, g_fs_delegate_request_type, uint64_t)
{
    ++mutations;
    return G_FS_MUTATION_READ_ONLY;
}
#include "production/filesystem_path_mutations.inc"

TEST(FilesystemMutation, PreservesLookupFailuresWithoutCallingTheBackend)
{
    delegate = {mutationCreate, mutationRemove};
    mutations = 0;
    for(auto status : {G_FS_OPEN_NOT_FOUND, G_FS_OPEN_BUSY, G_FS_OPEN_ERROR})
    {
        lookupStatus = status;
        auto expected = status == G_FS_OPEN_BUSY ? G_FS_MUTATION_BUSY : G_FS_MUTATION_ERROR;
        if(status == G_FS_OPEN_NOT_FOUND) expected = G_FS_MUTATION_NOT_FOUND;

        EXPECT_EQ(expected, filesystemRemove(&task, "child", false));
        if(status != G_FS_OPEN_NOT_FOUND)
            EXPECT_EQ(expected, filesystemMakeDirectory(&task, "child"));
    }

    EXPECT_EQ(0u, mutations);
}

TEST(FilesystemMutation, PreservesBackendResultsAfterSuccessfulPathResolution)
{
    delegate = {mutationCreate, mutationRemove};
    mutations = 0;
    lookupStatus = G_FS_OPEN_NOT_FOUND;

    EXPECT_EQ(G_FS_MUTATION_NO_SPACE, filesystemMakeDirectory(&task, "child"));

    lookupStatus = G_FS_OPEN_SUCCESSFUL;

    EXPECT_EQ(G_FS_MUTATION_READ_ONLY, filesystemRemove(&task, "child", false));
    EXPECT_EQ(2u, mutations);
}
TEST(FilesystemMutation, RemovesLinkEntriesWithoutTreatingDirectoryTargetsAsFolders)
{
    delegate = {mutationCreate, mutationRemove};
    mutations = 0;
    lookupStatus = G_FS_OPEN_SUCCESSFUL;
    file.type = G_FS_NODE_TYPE_SYMLINK;
    EXPECT_EQ(G_FS_MUTATION_NOT_DIRECTORY, filesystemRemove(&task, "directory.link", true));
    EXPECT_EQ(0u, mutations);
    EXPECT_EQ(G_FS_MUTATION_READ_ONLY, filesystemRemove(&task, "directory.link", false));
    EXPECT_EQ(1u, mutations);
    file.type = G_FS_NODE_TYPE_FILE;
}

}
