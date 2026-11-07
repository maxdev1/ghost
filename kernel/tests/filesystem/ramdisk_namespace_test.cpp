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
#include <ghost/filesystem/link.h>
#include "support/synchronization.hpp"
#include "production/filesystem_delegate_mutex.inc"
struct g_task;
#include "production/filesystem_delegate_layout.inc"
using g_ramdisk_entry_type = uint8_t;
using g_ramdisk_id = uint32_t;
constexpr g_ramdisk_entry_type G_RAMDISK_ENTRY_TYPE_FILE = 1;
#include "production/ramdisk_entry.inc"

namespace
{
static g_fs_delegate backend;
static g_ramdisk_entry rootEntry, physical;
static bool exists;
static unsigned creations;
static std::map<std::string, g_fs_node*> published;
static std::vector<g_fs_node*> nodes;
g_fs_delegate* filesystemFindDelegate(g_fs_node*) { return &backend; }
g_ramdisk_entry* ramdiskGetRoot() { return &rootEntry; }
g_ramdisk_entry* ramdiskFindById(g_fs_phys_id id) { return id == 0 ? &rootEntry : &physical; }
g_ramdisk_entry* ramdiskFindChild(g_ramdisk_entry*, const char*) { return exists ? &physical : nullptr; }
g_ramdisk_entry* ramdiskCreateFile(g_ramdisk_entry*, const char*)
{
	++creations; exists = true; physical.id = creations;
	return &physical;
}
bool filesystemFindExistingChild(g_fs_node*, const char* name, g_fs_node** out)
{
	auto found = published.find(name);
	*out = found == published.end() ? nullptr : found->second;
	return *out != nullptr;
}
g_fs_node* filesystemCreateNode(g_fs_node_type type, const char* name)
{
	auto node = new g_fs_node{};
	node->type = type; node->name = strdup(name);
	nodes.push_back(node);
	return node;
}
void filesystemAddChild(g_fs_node* parent, g_fs_node* node)
{
	node->parent = parent;
	published[node->name] = node;
}
#include "production/ramdisk_namespace.inc"

class RamdiskNamespace : public testing::Test
{
protected:
	g_fs_node root{};
	void SetUp() override
	{
		exists = false; creations = 0; published.clear();
		root.type = G_FS_NODE_TYPE_ROOT;
		physical.type = G_RAMDISK_ENTRY_TYPE_FILE; physical.id = 1;
	}
	void TearDown() override
	{
		for(auto node : nodes) { free(node->name); delete node; }
		nodes.clear(); published.clear();
	}
};

TEST_F(RamdiskNamespace, SerializesTwoInitialLookupMissesOntoOneCachedIdentity)
{
	exists = true;
	g_fs_node *first = nullptr, *second = nullptr;
	std::thread a([&] { EXPECT_EQ(G_FS_OPEN_SUCCESSFUL, filesystemRamdiskDelegateLookup(&root, "file", &first)); });
	std::thread b([&] { EXPECT_EQ(G_FS_OPEN_SUCCESSFUL, filesystemRamdiskDelegateLookup(&root, "file", &second)); });
	a.join(); b.join();
	EXPECT_EQ(first, second); EXPECT_EQ(1u, nodes.size());
}

TEST_F(RamdiskNamespace, ConcurrentCreationsCannotPublishDuplicatePhysicalEntries)
{
	g_fs_node *first = nullptr, *second = nullptr;
	g_fs_open_status one = G_FS_OPEN_ERROR, two = G_FS_OPEN_ERROR;
	std::thread a([&] { one = filesystemRamdiskDelegateCreate(&root, "file", &first); });
	std::thread b([&] { two = filesystemRamdiskDelegateCreate(&root, "file", &second); });
	a.join(); b.join();
	EXPECT_TRUE((one == G_FS_OPEN_SUCCESSFUL && two == G_FS_OPEN_ALREADY_EXISTS) ||
		(two == G_FS_OPEN_SUCCESSFUL && one == G_FS_OPEN_ALREADY_EXISTS));
	EXPECT_EQ(1u, creations); EXPECT_EQ(1u, nodes.size());
	EXPECT_TRUE((first == nullptr) != (second == nullptr));
}
}
