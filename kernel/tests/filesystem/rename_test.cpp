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
#include <ghost/filesystem/link.h>
#include "production/filesystem_link_format.inc"
#include "production/filesystem_delegate_mutex.inc"
struct g_task {};
#include "production/filesystem_delegate_layout.inc"
#include "support/tracked_heap.hpp"

namespace
{
struct g_filesystem_find_result
{
	g_fs_open_status status;
	g_fs_node* node;
	bool foundAllButLast;
	g_fs_node* lastFoundParent;
	const char* fileNameStart;
};
static std::map<std::string, g_fs_node*> paths;
static unsigned renameCalls;
static int failAllocation;
static g_fs_mutation_status renameResult;
void* heapTryAllocate(size_t size)
{
	if(failAllocation == 0)
		return nullptr;
	if(failAllocation > 0)
		--failAllocation;
	return heapAllocate(size);
}
size_t stringLength(const char* text) { return strlen(text); }
void stringCopy(char* target, const char* text) { strcpy(target, text); }
g_fs_delegate* filesystemFindDelegate(g_fs_node* node) { return node->delegate; }
g_fs_open_status filesystemMutationFind(g_task*, const char* path, g_filesystem_find_result* out)
{
	auto found = paths.find(path);
	if(found != paths.end())
	{
		*out = {G_FS_OPEN_SUCCESSFUL, found->second, false, found->second->parent, path};
		return G_FS_OPEN_SUCCESSFUL;
	}
	auto slash = strrchr(path, '/');
	std::string parent(path, slash ? size_t(slash - path) : 0);
	if(parent.empty())
		parent = "/";
	auto directory = paths.find(parent);
	*out = {G_FS_OPEN_NOT_FOUND, nullptr, directory != paths.end(),
		directory == paths.end() ? nullptr : directory->second, slash ? slash + 1 : path};
	return G_FS_OPEN_NOT_FOUND;
}
g_fs_mutation_status filesystemMutationLookupStatus(g_fs_open_status status)
{
	return status == G_FS_OPEN_NOT_FOUND ? G_FS_MUTATION_NOT_FOUND : G_FS_MUTATION_ERROR;
}
void filesystemPublishNode(g_fs_node* node, g_fs_node_entry* entry)
{
	entry->node = node;
	entry->next = node->parent->children;
	node->parent->children = entry;
}
#include "production/filesystem_unlink_node.inc"
#include "production/filesystem_rename.inc"

g_fs_mutation_status renameBackend(g_fs_node* node, g_fs_node*, const char*, g_fs_node*)
{
	EXPECT_GT(node->delegate->lock.depth, 0);
	EXPECT_FALSE(node->removed);
	++renameCalls;
	return renameResult;
}

class FilesystemRename : public testing::Test
{
protected:
	g_task task;
	g_fs_delegate delegate{};
	g_fs_node root{}, source{}, destination{}, child{};
	void SetUp() override
	{
		delegate.rename = renameBackend;
		root.type = G_FS_NODE_TYPE_ROOT;
		root.delegate = &delegate;
		source.name = stringDuplicate("source");
		source.type = G_FS_NODE_TYPE_FILE;
		source.physicalId = UINT64_C(0x100000001);
		source.parent = &root;
		source.delegate = &delegate;
		destination.name = stringDuplicate("destination");
		destination.type = G_FS_NODE_TYPE_FILE;
		destination.parent = &root;
		destination.delegate = &delegate;
		filesystemPublishNode(&source, static_cast<g_fs_node_entry*>(heapAllocate(sizeof(g_fs_node_entry))));
		filesystemPublishNode(&destination, static_cast<g_fs_node_entry*>(heapAllocate(sizeof(g_fs_node_entry))));
		paths = {{"/", &root}, {"/source", &source}, {"/destination", &destination}};
		renameCalls = 0;
		renameResult = G_FS_MUTATION_SUCCESSFUL;
		failAllocation = -1;
	}
	void TearDown() override
	{
		for(auto node: {&root, &source, &destination})
		{
			while(node->children)
			{
				auto entry = node->children;
				node->children = entry->next;
				heapFree(entry);
			}
		}
		heapFree(source.name);
		heapFree(destination.name);
		EXPECT_TRUE(heapBlocks.empty());
	}
};

TEST_F(FilesystemRename, RenamesAndPreservesNodeIdentityAndDescendantParents)
{
	source.type = G_FS_NODE_TYPE_FOLDER;
	child.parent = &source;
	ASSERT_EQ(G_FS_MUTATION_SUCCESSFUL, filesystemRename(&task, "/source", "/renamed", false));
	EXPECT_STREQ("renamed", source.name);
	EXPECT_EQ(UINT64_C(0x100000001), source.physicalId);
	EXPECT_EQ(&source, child.parent);
	EXPECT_FALSE(source.removed);
	EXPECT_EQ(1u, renameCalls);
}

TEST_F(FilesystemRename, ReplacesOnlyAnExplicitRegularFileAndRetiresItsCachedName)
{
	EXPECT_EQ(G_FS_MUTATION_ALREADY_EXISTS, filesystemRename(&task, "/source", "/destination", false));
	EXPECT_EQ(0u, renameCalls);
	ASSERT_EQ(G_FS_MUTATION_SUCCESSFUL, filesystemRename(&task, "/source", "/destination", true));
	EXPECT_TRUE(destination.removed);
	EXPECT_STREQ("destination", source.name);
	EXPECT_EQ(&root, source.parent);
	EXPECT_EQ(&source, root.children->node);
	EXPECT_EQ(nullptr, root.children->next);
}

TEST_F(FilesystemRename, RejectsDirectoryCollisionsDescendantsAndDifferentDelegates)
{
	source.type = G_FS_NODE_TYPE_FOLDER;
	destination.type = G_FS_NODE_TYPE_FOLDER;
	EXPECT_EQ(G_FS_MUTATION_ALREADY_EXISTS, filesystemRename(&task, "/source", "/destination", true));
	EXPECT_EQ(G_FS_MUTATION_INVALID_ARGUMENT, filesystemRename(&task, "/source", "/source/child", false));
	g_fs_delegate other{};
	destination.delegate = &other;
	EXPECT_EQ(G_FS_MUTATION_CROSS_DEVICE, filesystemRename(&task, "/source", "/destination/child", false));
	EXPECT_EQ(0u, renameCalls);
}

TEST_F(FilesystemRename, BackendAndAllocationFailuresKeepCachedNamesAndLinks)
{
	renameResult = G_FS_MUTATION_READ_ONLY;
	EXPECT_EQ(renameResult, filesystemRename(&task, "/source", "/renamed", false));
	EXPECT_STREQ("source", source.name);
	EXPECT_FALSE(source.removed);
	for(int allocation: {0, 1})
	{
		failAllocation = allocation;
		EXPECT_EQ(G_FS_MUTATION_ERROR, filesystemRename(&task, "/source", "/renamed", false));
		EXPECT_STREQ("source", source.name);
	}
	EXPECT_EQ(1u, renameCalls);
}
TEST_F(FilesystemRename, RenamingAcrossTheReservedSuffixUpdatesTheCachedType)
{
	ASSERT_EQ(G_FS_MUTATION_SUCCESSFUL, filesystemRename(&task, "/source", "/alias.link", false));
	EXPECT_EQ(G_FS_NODE_TYPE_SYMLINK, source.type);
	paths.erase("/source"); paths["/alias.link"] = &source;
	ASSERT_EQ(G_FS_MUTATION_SUCCESSFUL, filesystemRename(&task, "/alias.link", "/ordinary", false));
	EXPECT_EQ(G_FS_NODE_TYPE_FILE, source.type);
	EXPECT_STREQ("ordinary", source.name);
}

TEST_F(FilesystemRename, SameNodeOnlySucceedsWhenReplacementIsRequested)
{
	auto name = source.name;
	auto entries = root.children;
	EXPECT_EQ(G_FS_MUTATION_ALREADY_EXISTS, filesystemRename(&task, "/source", "/source", false));
	EXPECT_EQ(G_FS_MUTATION_SUCCESSFUL, filesystemRename(&task, "/source", "/source", true));
	EXPECT_EQ(name, source.name);
	EXPECT_EQ(entries, root.children);
	EXPECT_EQ(0u, renameCalls);
	EXPECT_EQ(0, delegate.lock.depth);
}

TEST_F(FilesystemRename, ReplacementFailurePreservesBothCachedEntries)
{
	auto sourceName = source.name;
	auto destinationName = destination.name;
	auto entries = root.children;
	root.upToDate = true;
	renameResult = G_FS_MUTATION_READ_ONLY;

	EXPECT_EQ(renameResult, filesystemRename(&task, "/source", "/destination", true));
	EXPECT_EQ(sourceName, source.name);
	EXPECT_EQ(destinationName, destination.name);
	EXPECT_EQ(entries, root.children);
	EXPECT_EQ(&destination, entries->node);
	ASSERT_NE(nullptr, entries->next);
	EXPECT_EQ(&source, entries->next->node);
	EXPECT_FALSE(source.removed);
	EXPECT_FALSE(destination.removed);
	EXPECT_TRUE(root.upToDate);
	EXPECT_EQ(0, delegate.lock.depth);
}

TEST_F(FilesystemRename, MovesBetweenParentsAndInvalidatesBothDirectoryCaches)
{
	destination.type = G_FS_NODE_TYPE_FOLDER;
	root.upToDate = destination.upToDate = true;
	ASSERT_EQ(G_FS_MUTATION_SUCCESSFUL, filesystemRename(&task, "/source", "/destination/moved", false));
	EXPECT_STREQ("moved", source.name);
	EXPECT_EQ(&destination, source.parent);
	EXPECT_EQ(&destination, root.children->node);
	EXPECT_EQ(nullptr, root.children->next);
	ASSERT_NE(nullptr, destination.children);
	EXPECT_EQ(&source, destination.children->node);
	EXPECT_EQ(nullptr, destination.children->next);
	EXPECT_FALSE(source.removed);
	EXPECT_FALSE(root.upToDate);
	EXPECT_FALSE(destination.upToDate);
	EXPECT_EQ(0, delegate.lock.depth);
	EXPECT_EQ(0, source.lock.depth);
}

TEST_F(FilesystemRename, RejectsMissingParentsNonDirectoriesAndUnsupportedBackends)
{
	EXPECT_EQ(G_FS_MUTATION_NOT_FOUND, filesystemRename(&task, "/missing", "/renamed", false));
	EXPECT_EQ(G_FS_MUTATION_INVALID_ARGUMENT, filesystemRename(&task, "/source", "/missing/child", false));
	EXPECT_EQ(G_FS_MUTATION_INVALID_ARGUMENT, filesystemRename(&task, "/source", "/destination/child", false));

	delegate.rename = nullptr;
	EXPECT_EQ(G_FS_MUTATION_NOT_SUPPORTED, filesystemRename(&task, "/source", "/renamed", false));
	EXPECT_EQ(0u, renameCalls);
	EXPECT_STREQ("source", source.name);
	EXPECT_FALSE(source.removed);
	EXPECT_EQ(0, delegate.lock.depth);
}

}
