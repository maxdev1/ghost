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
#include "production/filesystem_link_format.inc"
#include <ghost/tasks/types.h>
#include "production/filesystem_delegate_mutex.inc"
struct g_task;
#include "production/filesystem_delegate_layout.inc"
#include "support/tracked_heap.hpp"

namespace
{
struct g_test_process { g_pid id = 1; } testProcess;
struct g_task { g_fs_node* cwd; g_test_process* process = &testProcess; };
struct g_filesystem_find_result
{
	g_fs_open_status status;
	g_fs_node* node;
	bool foundAllButLast;
	g_fs_node* lastFoundParent;
	char fileNameStart[G_FILENAME_MAX];
};
static std::vector<g_fs_node*> nodes;
static std::map<g_fs_node*, std::string> contents;
static g_fs_delegate backend;
static g_fs_node* root;
static unsigned reads, publications;
static int writeLimit;
static bool flushed;
void* heapTryAllocate(size_t size) { return heapAllocate(size); }
void* memoryCopy(void* to, const void* from, int32_t size) { return memcpy(to, from, size); }
size_t stringLength(const char* text) { return strlen(text); }
void stringCopy(char* to, const char* from) { strcpy(to, from); }
g_fs_virt_id filesystemGetNextNodeId() { static g_fs_virt_id next = 1; return next++; }
g_fs_node* filesystemGetRoot() { return root; }
g_fs_delegate* filesystemFindDelegate(g_fs_node* node) { return node->delegate; }

static g_fs_node* linkTestNode(g_fs_node* parent, const char* name, g_fs_node_type type, const std::string& data = "")
{
	auto node = new g_fs_node{};
	node->id = filesystemGetNextNodeId();
	node->physicalId = node->id;
	node->parent = parent;
	node->name = stringDuplicate(name);
	node->type = filesystemStorageNodeType(type, name);
	node->delegate = &backend;
	nodes.push_back(node);
	contents[node] = data;
	return node;
}

g_fs_open_status filesystemFindChild(g_fs_node* parent, const char* name, g_fs_node** out)
{
	if(!strcmp(name, ".")) { *out = parent; return G_FS_OPEN_SUCCESSFUL; }
	if(!strcmp(name, "..")) { *out = parent->parent ? parent->parent : parent; return G_FS_OPEN_SUCCESSFUL; }
	for(auto node: nodes)
		if(node->parent == parent && !node->removed && !strcmp(node->name, name))
		{ *out = node; return G_FS_OPEN_SUCCESSFUL; }
	return G_FS_OPEN_NOT_FOUND;
}
#include "production/filesystem_absolute_path.inc"
g_fs_open_status filesystemChooseOrigin(const char* path, g_task* task, g_fs_node*& origin)
{
	origin = path[0] == '/' ? root : task->cwd;
	return G_FS_OPEN_SUCCESSFUL;
}
g_fs_mutation_status filesystemRename(g_task*, const char*, const char*, bool);
#include "production/filesystem_links.inc"

g_fs_mutation_status filesystemRename(g_task* task, const char* source, const char* target, bool)
{
	auto from = filesystemFindPolicy(task->cwd, source, false);
	auto to = filesystemFindPolicy(task->cwd, target, false);
	if(from.status != G_FS_OPEN_SUCCESSFUL || to.status != G_FS_OPEN_NOT_FOUND || !to.foundAllButLast)
		return G_FS_MUTATION_ERROR;
	EXPECT_TRUE(flushed);
	EXPECT_GE(filesystemParseLink(contents[from.node].data(), contents[from.node].size(), nullptr), 0);
	heapFree(from.node->name);
	from.node->name = stringDuplicate(to.fileNameStart);
	from.node->parent = to.lastFoundParent;
	from.node->type = filesystemStorageNodeType(from.node->type, from.node->name);
	++publications;
	return G_FS_MUTATION_SUCCESSFUL;
}
g_fs_length_status filesystemGetLength(g_fs_node* node, uint64_t* length)
{
	*length = contents[node].size(); return G_FS_LENGTH_SUCCESSFUL;
}
#include "production/filesystem_link_stat.inc"


static unsigned createAttempts, openCalls;
static bool loseCreationRace, vanishedCreation;
g_fs_open_status filesystemCreateFile(g_fs_node* parent, const char* name, g_fs_node** out)
{
	++createAttempts;
	if(loseCreationRace || vanishedCreation)
	{
		loseCreationRace = false;
		if(!vanishedCreation) linkTestNode(parent, name, G_FS_NODE_TYPE_FILE);
		return G_FS_OPEN_ALREADY_EXISTS;
	}
	return backend.create(parent, name, out);
}
g_fs_open_status filesystemTruncate(g_fs_node*) { return G_FS_OPEN_SUCCESSFUL; }
g_fs_open_status filesystemOpenNodeFd(g_fs_node*, g_file_flag_mode, g_pid, g_fd* out)
{
	++openCalls; *out = 42; return G_FS_OPEN_SUCCESSFUL;
}
#include "production/filesystem_open_policy.inc"

class FilesystemLinks : public testing::Test
{
protected:
	g_task task;
	void SetUp() override
	{
		reads = publications = 0; writeLimit = -1; flushed = false;
		createAttempts = openCalls = 0; loseCreationRace = vanishedCreation = false;
		backend = {};
		backend.getLength = filesystemGetLength;
		backend.read = [](g_fs_node* node, uint8_t* out, uint64_t offset, uint64_t size, int64_t* count) {
			++reads;
			auto& data = contents[node];
			auto length = offset < data.size() ? std::min(uint64_t(data.size()) - offset, std::min(size, uint64_t(3))) : 0;
			memcpy(out, data.data() + offset, length); *count = length;
			return G_FS_READ_SUCCESSFUL;
		};
		backend.create = [](g_fs_node* parent, const char* name, g_fs_node** out) {
			*out = linkTestNode(parent, name, G_FS_NODE_TYPE_FILE); return G_FS_OPEN_SUCCESSFUL;
		};
		backend.open = [](g_fs_node*, g_file_flag_mode) { return G_FS_OPEN_SUCCESSFUL; };
		backend.close = [](g_fs_node*, g_file_flag_mode) { return G_FS_CLOSE_SUCCESSFUL; };
		backend.write = [](g_fs_node* node, uint8_t* data, uint64_t offset, uint64_t length, int64_t* count) {
			if(writeLimit == 0) return G_FS_WRITE_ERROR;
			if(writeLimit > 0) --writeLimit;
			auto size = std::min(length, uint64_t(2));
			contents[node].resize(offset + size);
			memcpy(contents[node].data() + offset, data, size); *count = size;
			return G_FS_WRITE_SUCCESSFUL;
		};
		backend.mutate = [](g_fs_node* node, g_fs_delegate_request_type type, uint64_t) {
			if(type == G_FS_DELEGATE_REQUEST_REMOVE) node->removed = true;
			if(type == G_FS_DELEGATE_REQUEST_FLUSH) flushed = true;
			return G_FS_MUTATION_SUCCESSFUL;
		};
		backend.rename = [](g_fs_node*, g_fs_node*, const char*, g_fs_node*) { return G_FS_MUTATION_SUCCESSFUL; };
		root = linkTestNode(nullptr, "", G_FS_NODE_TYPE_ROOT);
		task.cwd = root;
	}
	void TearDown() override
	{
		for(auto node: nodes) { heapFree(node->name); delete node; }
		nodes.clear(); contents.clear(); EXPECT_TRUE(heapBlocks.empty());
	}
	g_filesystem_find_result find(const char* path, bool follow = true) { return filesystemFindPolicy(task.cwd, path, follow); }
};

TEST(FilesystemLinkFormat, ClassifiesReservedSuffixesWithoutChangingDirectoryTypes)
{
	EXPECT_FALSE(filesystemIsLinkName(""));
	EXPECT_FALSE(filesystemIsLinkName("link"));
	EXPECT_FALSE(filesystemIsLinkName("file.link.txt"));
	EXPECT_TRUE(filesystemIsLinkName(".link"));
	EXPECT_TRUE(filesystemIsLinkName("file.LiNk"));

	EXPECT_EQ(G_FS_NODE_TYPE_SYMLINK, filesystemStorageNodeType(G_FS_NODE_TYPE_FILE, "file.link"));
	EXPECT_EQ(G_FS_NODE_TYPE_FILE, filesystemStorageNodeType(G_FS_NODE_TYPE_SYMLINK, "file"));
	EXPECT_EQ(G_FS_NODE_TYPE_FOLDER, filesystemStorageNodeType(G_FS_NODE_TYPE_FOLDER, "dir.link"));
	EXPECT_EQ(G_FS_NODE_TYPE_MOUNTPOINT, filesystemStorageNodeType(G_FS_NODE_TYPE_MOUNTPOINT, "mount.link"));
}

TEST(FilesystemLinkFormat, PreservesTargetBytesWithOptionalLineEndings)
{
	const std::string expected = "/dir/file name-\xc3\xa4";
	char target[G_PATH_MAX];
	for(const auto* ending : {"", "\n", "\r\n"})
	{
		auto stored = "target=" + expected + ending;
		ASSERT_EQ(expected.size(), filesystemParseLink(stored.data(), stored.size(), target));
		EXPECT_EQ(expected, target);
		EXPECT_EQ(expected.size(), filesystemParseLink(stored.data(), stored.size(), nullptr));
	}
}

TEST(FilesystemLinkFormat, BoundsTargetAndStorageLengths)
{
	char target[G_PATH_MAX];
	const std::string expected(G_PATH_MAX - 1, 'x');
	auto stored = "target=" + expected + "\r\n";
	ASSERT_EQ(G_PATH_MAX - 1, filesystemParseLink(stored.data(), stored.size(), target));
	EXPECT_EQ(expected, target);

	stored = "target=" + std::string(G_PATH_MAX, 'x');
	EXPECT_EQ(-1, filesystemParseLink(stored.data(), stored.size(), target));
	stored += "\r\n";
	EXPECT_EQ(-1, filesystemParseLink(stored.data(), stored.size(), target));
}

TEST_F(FilesystemLinks, RetriesAnOrdinaryCreationRaceButKeepsExclusiveCreationAndRetryBounds)
{
	g_fd fd;
	loseCreationRace = true;
	EXPECT_EQ(G_FS_OPEN_SUCCESSFUL, filesystemOpen("/created", G_FILE_FLAG_MODE_CREATE, &task, &fd));
	EXPECT_EQ(1u, createAttempts); EXPECT_EQ(1u, openCalls); EXPECT_EQ(42, fd);

	loseCreationRace = true;
	EXPECT_EQ(G_FS_OPEN_ALREADY_EXISTS, filesystemOpen("/exclusive", G_FILE_FLAG_MODE_CREATE | G_FILE_FLAG_MODE_EXCLUSIVE, &task, &fd));
	EXPECT_EQ(2u, createAttempts); EXPECT_EQ(1u, openCalls);

	createAttempts = 0; vanishedCreation = true;
	EXPECT_EQ(G_FS_OPEN_BUSY, filesystemOpen("/churn", G_FILE_FLAG_MODE_CREATE, &task, &fd));
	EXPECT_EQ(4u, createAttempts); EXPECT_EQ(1u, openCalls);
}

TEST_F(FilesystemLinks, BuildsDeepPathsWithoutRecursiveStackGrowthAndBoundsActualCopies)
{
	auto leaf = root;
	for(unsigned i = 0; i < 1400; ++i)
		leaf = linkTestNode(leaf, "a", G_FS_NODE_TYPE_FOLDER);

	EXPECT_EQ(2800, filesystemGetAbsolutePathLength(leaf));
	char path[G_PATH_MAX];
	ASSERT_EQ(2800, filesystemGetAbsolutePath(leaf, path, sizeof(path)));
	EXPECT_EQ(2800u, strlen(path));

	char small[8]; memset(small, 'x', sizeof(small));
	EXPECT_EQ(-1, filesystemGetAbsolutePath(leaf, small, 4));
	EXPECT_EQ(0, small[0]); EXPECT_EQ('x', small[4]);

	leaf->parent = leaf;
	EXPECT_EQ(G_PATH_MAX, filesystemGetAbsolutePathLength(leaf));
	EXPECT_EQ(-1, filesystemGetAbsolutePath(leaf, path, sizeof(path)));
}

TEST_F(FilesystemLinks, FollowsAbsoluteRelativeIntermediateAndCrossMountTargetsWithoutHidingSuffixes)
{
	auto folder = linkTestNode(root, "folder", G_FS_NODE_TYPE_FOLDER);
	auto file = linkTestNode(folder, "file", G_FS_NODE_TYPE_FILE, "payload");
	linkTestNode(root, "abs.link", G_FS_NODE_TYPE_FILE, "target=/folder/file\n");
	linkTestNode(folder, "relative.link", G_FS_NODE_TYPE_FILE, "target=file\r\n");
	linkTestNode(root, "dir.link", G_FS_NODE_TYPE_FILE, "target=folder");
	auto mount = linkTestNode(root, "volume", G_FS_NODE_TYPE_MOUNTPOINT);
	auto external = linkTestNode(mount, "external", G_FS_NODE_TYPE_FILE);
	linkTestNode(folder, "cross.link", G_FS_NODE_TYPE_FILE, "target=/volume/external\n");
	EXPECT_EQ(file, find("/abs.link").node);
	EXPECT_EQ(file, find("/folder/relative.link").node);
	EXPECT_EQ(file, find("/dir.link/file").node);
	EXPECT_EQ(external, find("/folder/cross.link").node);
	EXPECT_EQ(G_FS_OPEN_NOT_FOUND, find("/abs").status);
	EXPECT_EQ(G_FS_NODE_TYPE_SYMLINK, find("/abs.link", false).node->type);
}

TEST_F(FilesystemLinks, ResolvesDotDotAfterThePrecedingLinkAndRejectsNonDirectoryComponents)
{
	auto tree = linkTestNode(root, "tree", G_FS_NODE_TYPE_FOLDER);
	auto branch = linkTestNode(tree, "branch", G_FS_NODE_TYPE_FOLDER);
	auto expected = linkTestNode(tree, "value", G_FS_NODE_TYPE_FILE);
	linkTestNode(root, "value", G_FS_NODE_TYPE_FILE);
	linkTestNode(root, "dir.link", G_FS_NODE_TYPE_FILE, "target=tree/branch\n");
	EXPECT_EQ(expected, find("dir.link/../value").node);
	EXPECT_EQ(branch, find("dir.link/.").node);
	EXPECT_EQ(G_FS_OPEN_NOT_DIRECTORY, find("value/../value").status);
	EXPECT_EQ(G_FS_OPEN_NOT_DIRECTORY, find("dir.link/", false).status);
}

TEST_F(FilesystemLinks, DanglingLinksKeepTheirTargetsAndOwnTheExpandedCreationName)
{
	auto node = linkTestNode(root, "missing.link", G_FS_NODE_TYPE_FILE, "target=missing\n");
	auto found = find("missing.link");
	EXPECT_EQ(G_FS_OPEN_NOT_FOUND, found.status);
	EXPECT_TRUE(found.foundAllButLast);
	EXPECT_STREQ("missing", found.fileNameStart);
	char target[G_PATH_MAX]; uint32_t length = 0;
	EXPECT_EQ(G_FS_OPEN_SUCCESSFUL, filesystemReadLink(node, target, &length));
	EXPECT_STREQ("missing", target);
	EXPECT_EQ(node, find("missing.link", false).node);
}

TEST_F(FilesystemLinks, DetectsLoopsAndBoundsTargetExpansionAndComponents)
{
	linkTestNode(root, "a.link", G_FS_NODE_TYPE_FILE, "target=b.link\n");
	linkTestNode(root, "b.link", G_FS_NODE_TYPE_FILE, "target=a.link\n");
	EXPECT_EQ(G_FS_OPEN_LOOP, find("a.link").status);
	EXPECT_LE(reads, 40u * 10);
	std::string large(G_PATH_MAX, 'x');
	EXPECT_EQ(G_FS_OPEN_NAME_TOO_LONG, find(large.c_str()).status);
	large.resize(G_FILENAME_MAX);
	EXPECT_EQ(G_FS_OPEN_NAME_TOO_LONG, find(large.c_str()).status);
	linkTestNode(root, "big.link", G_FS_NODE_TYPE_FILE, "target=" + std::string(G_PATH_MAX - 1, 'x'));
	EXPECT_EQ(G_FS_OPEN_NAME_TOO_LONG, find("big.link/suffix").status);
}

TEST_F(FilesystemLinks, MalformedLinksRemainInspectableAndNeverFallBackToRegularFiles)
{
	auto node = linkTestNode(root, "bad.LINK", G_FS_NODE_TYPE_FILE);
	for(auto text: {std::string{}, std::string("target="), std::string("other=x"), std::string("target=x\nextra=x"),
		std::string("target=x\n\n"), std::string("target=a\0b", 10)})
	{
		contents[node] = text;
		EXPECT_EQ(node, find("bad.LINK", false).node);
		EXPECT_EQ(G_FS_OPEN_INVALID_LINK, find("bad.LINK").status);
		g_fs_stat_data data{};
		EXPECT_EQ(G_FS_STAT_SUCCESS, filesystemStat(&task, "bad.LINK", &data, false));
		EXPECT_EQ(G_FS_NODE_TYPE_SYMLINK, data.type);
		EXPECT_EQ(0u, data.size);
		EXPECT_EQ(G_FS_STAT_INVALID_LINK, filesystemStat(&task, "bad.LINK", &data, true));
	}
}

TEST_F(FilesystemLinks, StatAndFreshReadsObserveRawContentReplacementWithoutCachedTargets)
{
	auto first = linkTestNode(root, "one", G_FS_NODE_TYPE_FILE, "one");
	auto second = linkTestNode(root, "two", G_FS_NODE_TYPE_FILE, "two");
	auto link = linkTestNode(root, "alias.link", G_FS_NODE_TYPE_FILE, "target=one\n");
	EXPECT_EQ(first, find("alias.link").node);
	contents[link] = "target=two\n";
	EXPECT_EQ(second, find("alias.link").node);
	g_fs_stat_data info{};
	ASSERT_EQ(G_FS_STAT_SUCCESS, filesystemStat(&task, "alias.link", &info, false));
	EXPECT_EQ(G_FS_NODE_TYPE_SYMLINK, info.type); EXPECT_EQ(3u, info.size);
	ASSERT_EQ(G_FS_STAT_SUCCESS, filesystemStat(&task, "alias.link", &info, true));
	EXPECT_EQ(second->id, info.virtual_id);
	EXPECT_EQ(G_FS_NODE_TYPE_FILE, info.type);
}

TEST_F(FilesystemLinks, CreatesOnlyFullyWrittenFlushedLinksAndCleansUpOnFailure)
{
	ASSERT_EQ(G_FS_MUTATION_SUCCESSFUL, filesystemSymlink(&task, "dangling target", "/created.link"));
	EXPECT_EQ(1u, publications);
	EXPECT_EQ("target=dangling target\n", contents[find("created.link", false).node]);
	EXPECT_EQ(G_FS_MUTATION_ALREADY_EXISTS, filesystemSymlink(&task, "other", "/created.link"));
	EXPECT_EQ(G_FS_MUTATION_INVALID_ARGUMENT, filesystemSymlink(&task, "other", "/wrong"));
	EXPECT_EQ(G_FS_MUTATION_INVALID_ARGUMENT, filesystemSymlink(&task, "a\nb", "/bad.link"));
	writeLimit = 1;
	EXPECT_EQ(G_FS_MUTATION_ERROR, filesystemSymlink(&task, "target", "/failed.link"));
	EXPECT_EQ(G_FS_OPEN_NOT_FOUND, find("failed.link", false).status);
	for(auto node: nodes)
		if(!strncmp(node->name, ".ghost-link-", 12)) EXPECT_TRUE(node->removed);
}

TEST_F(FilesystemLinks, RejectsMissingParentsAndRemovesLinksThatCannotClose)
{
	EXPECT_EQ(G_FS_MUTATION_NOT_FOUND, filesystemSymlink(&task, "target", "/missing/created.link"));
	backend.close = [](g_fs_node*, g_file_flag_mode) { return G_FS_CLOSE_ERROR; };

	EXPECT_EQ(G_FS_MUTATION_ERROR, filesystemSymlink(&task, "target", "/created.link"));
	EXPECT_EQ(0u, publications);
	EXPECT_EQ(G_FS_OPEN_NOT_FOUND, find("created.link", false).status);
	for(auto node: nodes)
		if(!strncmp(node->name, ".ghost-link-", 12))
			EXPECT_TRUE(node->removed);
}

TEST_F(FilesystemLinks, AcceptsExactlyTheTraversalLimitAndRejectsOneMoreLink)
{
	auto file = linkTestNode(root, "end", G_FS_NODE_TYPE_FILE);
	for(int i = G_FS_LINK_LIMIT - 1; i >= 0; --i)
	{
		auto name = "chain" + std::to_string(i) + ".link";
		auto target = i == G_FS_LINK_LIMIT - 1 ? "end" : "chain" + std::to_string(i + 1) + ".link";
		linkTestNode(root, name.c_str(), G_FS_NODE_TYPE_FILE, "target=" + target + "\n");
	}
	EXPECT_EQ(file, find("chain0.link").node);
	linkTestNode(root, "extra.link", G_FS_NODE_TYPE_FILE, "target=chain0.link\n");
	EXPECT_EQ(G_FS_OPEN_LOOP, find("extra.link").status);
}

TEST_F(FilesystemLinks, PreservesCreationDetailsThroughRepeatedDirectoryLinkExpansion)
{
	auto folder = linkTestNode(root, "folder", G_FS_NODE_TYPE_FOLDER);
	auto directory = linkTestNode(folder, "directory", G_FS_NODE_TYPE_FOLDER);
	linkTestNode(root, "first.link", G_FS_NODE_TYPE_FILE, "target=folder/second.link\n");
	linkTestNode(folder, "second.link", G_FS_NODE_TYPE_FILE, "target=directory\n");

	auto found = find("first.link///missing");
	EXPECT_EQ(G_FS_OPEN_NOT_FOUND, found.status);
	EXPECT_TRUE(found.foundAllButLast);
	EXPECT_EQ(directory, found.lastFoundParent);
	EXPECT_STREQ("missing", found.fileNameStart);
	EXPECT_FALSE(find("first.link/missing/").foundAllButLast);
	EXPECT_FALSE(find("first.link/missing/child").foundAllButLast);

	EXPECT_EQ(G_FS_OPEN_NOT_FOUND, find(nullptr).status);
	EXPECT_EQ(G_FS_OPEN_NOT_FOUND, find("").status);
	EXPECT_EQ(root, find("///").node);
}

static g_fs_mutation_status linkTestFailFlush(g_fs_node* node, g_fs_delegate_request_type type, uint64_t)
{
	if(type == G_FS_DELEGATE_REQUEST_REMOVE)
	{
		node->removed = true;
		return G_FS_MUTATION_SUCCESSFUL;
	}
	return G_FS_MUTATION_READ_ONLY;
}

TEST_F(FilesystemLinks, RemovesTemporaryStorageAndPreservesFlushErrors)
{
	backend.mutate = linkTestFailFlush;
	EXPECT_EQ(G_FS_MUTATION_READ_ONLY, filesystemSymlink(&task, "target", "/created.link"));
	EXPECT_EQ(0u, publications);
	EXPECT_EQ(G_FS_OPEN_NOT_FOUND, find("created.link", false).status);
	ASSERT_EQ(2u, nodes.size());
	EXPECT_TRUE(nodes.back()->removed);
}

TEST_F(FilesystemLinks, RejectsUnsupportedPublicationBeforeCreatingTemporaryStorage)
{
	backend.rename = nullptr;
	EXPECT_EQ(G_FS_MUTATION_NOT_SUPPORTED, filesystemSymlink(&task, "target", "/created.link"));
	EXPECT_EQ(0u, publications);
	EXPECT_EQ(1u, nodes.size());
}

}
