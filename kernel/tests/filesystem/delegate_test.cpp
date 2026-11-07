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
#include <functional>
#include <set>
#define GHOST_API_STDINT
#define GHOST_API_MEMORY_TYPES
using g_bool = uint8_t;
#define G_PAGE_SIZE 4096ULL
#define G_PAGE_ALIGN_MASK 4095ULL
#include "../../../libapi/inc/ghost/filesystem/callstructs.h"
#include <ghost/filesystem/link.h>
#include "production/filesystem_link_format.inc"
#include "../../../libapi/inc/ghost/syscall/definitions.h"
#include "production/filesystem_delegate_mutex.inc"
struct g_task;
#include "production/filesystem_delegate_layout.inc"

static constexpr uint64_t G_MEM_LOWER_HALF_END = 0x7fffffffffffULL;
static constexpr uint64_t G_PAGE_PRESENT = 1, G_PAGE_WRITABLE_FLAG = 2, G_PAGE_USER_FLAG = 4;
static constexpr uint64_t G_PAGE_UNMANAGED_FLAG = 1 << 9, G_PAGE_LARGE_PAGE_FLAG = 1 << 7;
static constexpr uint64_t G_PAGE_PHYSICAL_MASK = 0x000ffffffffff000ULL;
static constexpr uint64_t G_PAGE_TABLE_USER_DEFAULT = 7, G_PAGE_USER_DEFAULT = 7;
static constexpr uint8_t G_PROC_VIRTUAL_RANGE_FLAG_INTERNAL = 2;
static constexpr int G_MESSAGE_TRANSACTION_NONE = -1;
static constexpr int G_MESSAGE_SEND_STATUS_SUCCESSFUL = 1, G_MESSAGE_SEND_STATUS_FULL = 2;
#define G_MEM_PHYS_TO_VIRT(value) (value)

struct g_address_range_pool { uintptr_t next = 0xa0000000; std::map<uintptr_t, uint8_t> ranges; };
struct g_process { g_mutex lock{}; uintptr_t pageSpace; g_address_range_pool* virtualRangePool; bool terminating; };
struct g_task {
    g_process* process;
    g_tid id;
    g_security_level securityLevel = G_SECURITY_LEVEL_DRIVER;
    bool terminationRequested = false;
    bool alive = true;
    unsigned references = 0;
};

struct g_wait_queue { std::set<g_tid> tasks; };
struct g_clock_local { uint64_t time = 0; } delegateClock;
struct g_interrupt_guard {};
#include "support/tracked_heap.hpp"

void mutexInitializeTask(g_mutex* mutex, const char* = nullptr) { mutex->depth = 0; }

static g_task* currentTask;
static std::map<g_tid, g_task*> tasks;
static std::map<uintptr_t, std::map<uintptr_t, uint64_t>> spaces;
static std::map<uintptr_t, unsigned> frames;
static uintptr_t currentSpace;
static int failFrame = -1, failMap = -1, failHeap = -1;
static bool failRange;
static uint64_t wakeAt;
static std::function<void()> serve;
static g_fs_delegate_request notification;
static bool failSend;
static unsigned notifications;
static std::map<g_fs_virt_id, g_fs_node*> nodes;
static auto filesystemNodes = &nodes;
static g_fs_node* mountFolder;
static g_fs_virt_id nextNode;

void* heapTryAllocate(size_t size) {
    if(failHeap == 0) return nullptr;
    if(failHeap > 0) --failHeap;
    return heapAllocate(size);
}

void* memoryCopy(void* out, const void* in, size_t size) { return std::memcpy(out, in, size); }
void* memorySetBytes(void* out, uint8_t value, size_t size) { return std::memset(out, value, size); }
size_t stringLength(const char* text) { return std::strlen(text); }
int stringCopy(char* out, const char* in) { std::strcpy(out, in); return std::strlen(in); }
g_fs_virt_id filesystemGetNextNodeId() { return nextNode++; }
void hashmapPut(std::map<g_fs_virt_id, g_fs_node*>* map, g_fs_virt_id id, g_fs_node* node) { (*map)[id] = node; }
g_fs_node* filesystemGetNode(g_fs_virt_id id) { auto it = nodes.find(id); return it == nodes.end() ? nullptr : it->second; }
bool filesystemFindExistingChild(g_fs_node* parent, const char* name, g_fs_node** out = nullptr);
#include "production/filesystem_find_existing_child.inc"

g_task* taskingGetCurrentTask() { return currentTask; }
g_task* taskingAcquireTask(g_tid id) {
    auto it = tasks.find(id);
    if(it == tasks.end() || !it->second->alive) return nullptr;

    ++it->second->references;
    return it->second;
}

void taskingReleaseTask(g_task* task) { if(task) { EXPECT_GT(task->references, 0U); --task->references; } }
void waitQueueInitialize(g_wait_queue* queue) { queue->tasks.clear(); }
void waitQueueAdd(g_wait_queue* queue, g_tid id) { queue->tasks.insert(id); }
void waitQueueRemove(g_wait_queue* queue, g_tid id) { queue->tasks.erase(id); }
void waitQueueWake(g_wait_queue*) {}
void taskingPrepareWait(g_task*, const char*) {}
g_clock_local* clockGetLocal() { return &delegateClock; }
uint64_t clockGetMilliseconds() { return clockGetLocal()->time; }
void clockWaitForTime(g_tid, uint64_t time) { wakeAt = time; }
void clockUnwaitForTime(g_task*) {}
void taskingYield();

g_physical_address memoryPhysicalTryAllocate() {
    if(failFrame == 0) return 0;
    if(failFrame > 0) --failFrame;
    void* page = nullptr;
    if(posix_memalign(&page, G_PAGE_SIZE, G_PAGE_SIZE)) return 0;

    frames[uintptr_t(page)] = 1;
    return uintptr_t(page);
}

void memoryPhysicalFree(g_physical_address page) {
    auto it = frames.find(page);
    ASSERT_NE(it, frames.end());
    if(!--it->second) { std::free(reinterpret_cast<void*>(page)); frames.erase(it); }
}

void pageReferenceTrackerIncrement(g_physical_address page) { ASSERT_TRUE(frames.count(page)); ++frames[page]; }
g_address addressRangePoolAllocate(g_address_range_pool* pool, uint32_t pages, uint8_t flags) {
    EXPECT_EQ(2U, pages);
    EXPECT_EQ(G_PROC_VIRTUAL_RANGE_FLAG_INTERNAL, flags);
    if(failRange) return 0;

    auto base = pool->next; pool->next += pages * G_PAGE_SIZE;
    pool->ranges[base] = flags;
    return base;
}

int addressRangePoolFree(g_address_range_pool* pool, g_address base) { EXPECT_EQ(1U, pool->ranges.erase(base)); return 2; }
bool pagingMapPage(g_virtual_address address, g_physical_address physical, uint64_t, uint64_t flags) {
    if(failMap == 0) return false;
    if(failMap > 0) --failMap;
    spaces[currentSpace][address] = physical | flags;
    return true;
}

void pagingUnmapPage(g_virtual_address address) { EXPECT_EQ(1U, spaces[currentSpace].erase(address)); }
uint64_t pagingVirtualToPageEntry(g_virtual_address address) {
    auto it = spaces[currentSpace].find(address & ~uintptr_t(G_PAGE_ALIGN_MASK));
    return it == spaces[currentSpace].end() ? 0 : it->second;
}

bool memoryOnDemandHandlePageFault(g_task*, g_address) { return false; }
static uintptr_t userStackPage;
bool taskingMemoryHandleStackOverflow(g_task* task, g_address address) {
    auto page = address & ~uintptr_t(G_PAGE_ALIGN_MASK);
    if(!userStackPage || page != userStackPage) return false;

    spaces[task->process->pageSpace][page] = page | 7;
    return true;
}

g_physical_address taskingMemoryTemporarySwitchTo(g_physical_address space) { auto old = currentSpace; currentSpace = space; return old; }
void taskingMemoryTemporarySwitchBack(g_physical_address space) { currentSpace = space; }
int messageQueueSend(g_tid sender, g_tid owner, void* bytes, uint32_t length, int transaction) {
    EXPECT_EQ(G_FS_DELEGATE_MESSAGE_SENDER, sender);
    EXPECT_EQ(G_MESSAGE_TRANSACTION_NONE, transaction);
    EXPECT_EQ(sizeof(notification), length);
    EXPECT_TRUE(tasks.count(owner));
    if(failSend) return G_MESSAGE_SEND_STATUS_FULL;

    std::memcpy(&notification, bytes, sizeof(notification));
    ++notifications;
    return G_MESSAGE_SEND_STATUS_SUCCESSFUL;
}

#include "production/filesystem_delegate_hpp.inc"
#include "production/filesystem_delegate_nodes.inc"
bool memoryUserCopyPinned(g_task*, g_address, void*, size_t, bool) { return false; }
#include "production/memory_user_copy.inc"
#include "support/memory_user.hpp"
#include "production/filesystem_delegate_memory.inc"
#include "production/filesystem_unlink_node.inc"
static g_fs_node* filesystemRoot;
g_fs_node* filesystemGetRoot() { return memoryAtomicLoad(&filesystemRoot, G_MEMORY_ORDER_ACQUIRE); }
#include "production/filesystem_adopt_root.inc"
#include "production/filesystem_absolute_path.inc"
#include "production/filesystem_delegate_cpp.inc"
#include "production/filesystem_delegate_syscalls.inc"

g_fs_read_status filesystemRead(g_task*, g_fd, void*, uint64_t, int64_t*) {
    ADD_FAILURE() << "Invalid syscall input reached filesystemRead";
    return G_FS_READ_ERROR;
}

g_fs_write_status filesystemWrite(g_task*, g_fd, const void*, uint64_t, int64_t*) {
    ADD_FAILURE() << "Invalid syscall input reached filesystemWrite";
    return G_FS_WRITE_ERROR;
}
#include "production/filesystem_delegate_io_syscalls.inc"
#include "support/syscall_dispatch.hpp"


void taskingYield() {
    EXPECT_EQ(0, filesystemDelegateLock.depth);
    auto caller = currentTask;
    auto space = currentSpace;
    if(serve) serve();
    currentTask = caller;
    currentSpace = space;
    delegateClock.time = std::max(delegateClock.time, wakeAt);
}

static void delegateTestMap(g_task* task, void* address, size_t size) {
    auto first = uintptr_t(address) & ~uintptr_t(G_PAGE_ALIGN_MASK);
    auto end = (uintptr_t(address) + size + G_PAGE_ALIGN_MASK) & ~uintptr_t(G_PAGE_ALIGN_MASK);
    for(auto page = first; page < end; page += G_PAGE_SIZE) spaces[task->process->pageSpace][page] = page | 7;
}

class FilesystemTaskDelegate : public testing::Test {
protected:
    g_address_range_pool serverRanges, clientRanges, strangerRanges;
    g_process serverProcess{{}, 1, &serverRanges, false}, clientProcess{{}, 2, &clientRanges, false},
        strangerProcess{{}, 3, &strangerRanges, false};
    g_task server{&serverProcess, 10}, client{&clientProcess, 20}, stranger{&strangerProcess, 30};
    g_fs_virt_id mount = G_FS_VIRT_ID_NONE;
    g_fs_delegate_transaction_storage* storage = nullptr;

    void select(g_task* task) { currentTask = task; currentSpace = task->process->pageSpace; }
    void SetUp() override {
        ASSERT_TRUE(heapBlocks.empty());
        ASSERT_TRUE(frames.empty());
        userStackPage = 0;
        spaces.clear(); tasks = {{10, &server}, {20, &client}, {30, &stranger}};
        failFrame = failMap = failHeap = -1; failRange = failSend = false;
        delegateClock.time = wakeAt = notifications = 0;
        filesystemDelegates = nullptr;
        filesystemDelegateActiveCount = filesystemDelegateRecordCount = filesystemDelegateNodeCount = 0;
        filesystemDelegateNextTransaction = UINT64_C(0x100000000);
        nextNode = 1;
        nodes.clear();
        filesystemDelegateInitialize();
        mountFolder = filesystemCreateDetachedNode(G_FS_NODE_TYPE_FOLDER, "mount");
        nodes[mountFolder->id] = mountFolder;
        filesystemRoot = filesystemCreateDetachedNode(G_FS_NODE_TYPE_ROOT, "root");
        nodes[filesystemRoot->id] = filesystemRoot;
        filesystemAddChild(filesystemRoot, mountFolder);
        select(&server);
    }

    void TearDown() override {
        serve = {};
        select(&server);
        filesystemDelegateTaskRemoved(&server);
        filesystemDelegateTaskRemoved(&client);
        filesystemDelegateTaskRemoved(&stranger);
        while(filesystemDelegates) {
            auto delegate = filesystemDelegates;
            filesystemDelegates = delegate->next;
            if(delegate->transaction) {
                filesystemDelegateDiscardPending(delegate->transaction);
                heapFree(delegate->transaction);
            }

            heapFree(delegate);
        }

        for(auto item : nodes) {
            while(item.second->children) {
                auto entry = item.second->children;
                item.second->children = entry->next;
                heapFree(entry);
            }

            filesystemDiscardDetachedNode(item.second);
        }

        nodes.clear();
        EXPECT_TRUE(frames.empty());
        EXPECT_TRUE(heapBlocks.empty());
        EXPECT_EQ(0U, server.references + client.references + stranger.references);
    }

    g_fs_delegate_status registerMount(const char* name = "fat0") {
        g_syscall_fs_delegate_register call{};
        call.name = name; call.phys_mountpoint_id = UINT64_C(0x200000000);
        call.version = G_FS_DELEGATE_VERSION; call.storage_size = sizeof(g_fs_delegate_transaction_storage);
        auto status = filesystemDelegateRegister(&server, &call);
        mount = call.mountpoint_id; storage = call.transaction_storage;
        return status;
    }

    g_fs_node* root() { return filesystemGetNode(mount); }
    g_fs_delegate_transaction_storage* shared() {
        return reinterpret_cast<g_fs_delegate_transaction_storage*>(G_MEM_PHYS_TO_VIRT(filesystemDelegates->controlPage));
    }

    uint8_t* io() { return reinterpret_cast<uint8_t*>(G_MEM_PHYS_TO_VIRT(filesystemDelegates->ioPage)); }
    void respond(const std::function<void()>& action) { serve = [&, action] { select(&server); action(); }; select(&client); }
};

TEST_F(FilesystemTaskDelegate, PromotesTheMatchingMountAndHandlesItsOwnerExit) {
    ASSERT_FALSE(filesystemDelegateMountRoot("fat-root"));
    ASSERT_EQ(G_FS_DELEGATE_SUCCESSFUL, registerMount("fat-root"));
    ASSERT_FALSE(filesystemDelegateMountRoot("other"));
    ASSERT_TRUE(filesystemDelegateMountRoot("fat-root"));
    EXPECT_EQ(root(), filesystemRoot);
    EXPECT_EQ(root(), mountFolder->parent);
    EXPECT_EQ(nullptr, root()->parent);
    EXPECT_EQ(nullptr, mountFolder->children);
    EXPECT_EQ(nullptr, filesystemDelegates->mountEntry);
    EXPECT_FALSE(filesystemDelegateMountRoot("fat-root"));

    filesystemDelegateTaskRemoved(&server);

    EXPECT_FALSE(filesystemDelegates->online);
}

TEST_F(FilesystemTaskDelegate, PromotedRootHasCanonicalPathsAndKeepsSyntheticMountsWhenRefreshed) {
    ASSERT_EQ(G_FS_DELEGATE_SUCCESSFUL, registerMount("fat-root"));
    ASSERT_TRUE(filesystemDelegateMountRoot("fat-root"));
    EXPECT_EQ(G_FS_NODE_TYPE_ROOT, root()->type);

    char path[128];

    EXPECT_EQ(1, filesystemGetAbsolutePathLength(root()));

    filesystemGetAbsolutePath(root(), path, sizeof(path));

    EXPECT_STREQ("/", path);

    filesystemGetAbsolutePath(mountFolder, path, sizeof(path));

    EXPECT_STREQ("/mount", path);

    respond([&] {
        ASSERT_EQ(G_FS_DELEGATE_REQUEST_READ_DIRECTORY, notification.type);

        g_syscall_fs_delegate_node_create call{};
        call.transaction = notification.transaction; call.parent_id = mount;
        call.name = "applications"; call.type = G_FS_NODE_TYPE_FOLDER; call.phys_fs_id = 1;

        ASSERT_EQ(G_FS_DELEGATE_SUCCESSFUL, filesystemDelegateCreateNode(&server, &call));

        shared()->data.directory_refresh.result_status = G_FS_DIRECTORY_REFRESH_SUCCESSFUL;

        EXPECT_EQ(G_FS_DELEGATE_SUCCESSFUL, filesystemDelegateComplete(&server, notification.transaction));
    });

    ASSERT_EQ(G_FS_DIRECTORY_REFRESH_SUCCESSFUL, root()->delegate->refreshDir(root()));

    g_fs_node* applications = nullptr;

    ASSERT_TRUE(filesystemFindExistingChild(root(), "applications", &applications));
    EXPECT_TRUE(filesystemFindExistingChild(root(), "mount"));
    EXPECT_EQ(13, filesystemGetAbsolutePathLength(applications));

    filesystemGetAbsolutePath(applications, path, sizeof(path));

    EXPECT_STREQ("/applications", path);

    g_fs_node* parent = nullptr;

    ASSERT_TRUE(filesystemFindExistingChild(root(), "..", &parent));
    EXPECT_EQ(root(), parent);
    ASSERT_TRUE(filesystemFindExistingChild(mountFolder, "..", &parent));
    EXPECT_EQ(root(), parent);
}

TEST_F(FilesystemTaskDelegate, PromotedRootSupportsDelegatedLookupAndFileCreation) {
    ASSERT_EQ(G_FS_DELEGATE_SUCCESSFUL, registerMount("fat-root"));
    ASSERT_TRUE(filesystemDelegateMountRoot("fat-root"));

    respond([&] {
        ASSERT_EQ(G_FS_DELEGATE_REQUEST_LOOKUP, notification.type);

        g_syscall_fs_delegate_node_create call{};
        call.transaction = notification.transaction; call.parent_id = mount;
        call.name = "system"; call.type = G_FS_NODE_TYPE_FOLDER; call.phys_fs_id = 1;

        ASSERT_EQ(G_FS_DELEGATE_SUCCESSFUL, filesystemDelegateCreateNode(&server, &call));

        shared()->data.lookup.result_node_id = call.created_id;
        shared()->data.lookup.result_status = G_FS_LOOKUP_SUCCESSFUL;

        EXPECT_EQ(G_FS_DELEGATE_SUCCESSFUL, filesystemDelegateComplete(&server, notification.transaction));
    });
    g_fs_node* found = nullptr;

    ASSERT_EQ(G_FS_OPEN_SUCCESSFUL, root()->delegate->lookup(root(), "system", &found));
    ASSERT_NE(nullptr, found);

    respond([&] {
        ASSERT_EQ(G_FS_DELEGATE_REQUEST_CREATE, notification.type);

        g_syscall_fs_delegate_node_create call{};
        call.transaction = notification.transaction; call.parent_id = mount;
        call.name = "created.txt"; call.type = G_FS_NODE_TYPE_FILE; call.phys_fs_id = 2;

        ASSERT_EQ(G_FS_DELEGATE_SUCCESSFUL, filesystemDelegateCreateNode(&server, &call));

        shared()->data.create.result_node_id = call.created_id;
        shared()->data.create.result_status = G_FS_MUTATION_SUCCESSFUL;

        EXPECT_EQ(G_FS_DELEGATE_SUCCESSFUL, filesystemDelegateComplete(&server, notification.transaction));
    });

    ASSERT_EQ(G_FS_OPEN_SUCCESSFUL, root()->delegate->create(root(), "created.txt", &found));
    ASSERT_NE(nullptr, found);
    EXPECT_STREQ("created.txt", found->name);
}

TEST_F(FilesystemTaskDelegate, RegistersOwnedPagesAndUnregistersWithoutRetargetingOldNodes) {
    ASSERT_EQ(G_FS_DELEGATE_SUCCESSFUL, registerMount());

    auto old = root();

    EXPECT_EQ(UINT64_C(0x200000000), old->physicalId);
    EXPECT_EQ(2U, frames.size());
    for(auto page : frames) EXPECT_EQ(2U, page.second);

    EXPECT_EQ(G_FS_DELEGATE_NOT_OWNER, filesystemDelegateUnregister(&stranger, mount));
    EXPECT_EQ(G_FS_DELEGATE_SUCCESSFUL, filesystemDelegateUnregister(&server, mount));
    EXPECT_TRUE(frames.empty());
    EXPECT_FALSE(filesystemFindExistingChild(mountFolder, "fat0"));
    ASSERT_EQ(G_FS_DELEGATE_SUCCESSFUL, registerMount());
    EXPECT_NE(old->id, mount);

    select(&client);

    EXPECT_EQ(G_FS_OPEN_ERROR, old->delegate->open(old, 0));
}

TEST_F(FilesystemTaskDelegate, RejectsDuplicateAndIncompatibleRegistrations) {
    ASSERT_EQ(G_FS_DELEGATE_SUCCESSFUL, registerMount());

    auto allocated = frames.size();

    EXPECT_EQ(G_FS_DELEGATE_ALREADY_EXISTS, registerMount());
    EXPECT_EQ(allocated, frames.size());

    g_syscall_fs_delegate_register call{};
    call.name = "other"; call.version = G_FS_DELEGATE_VERSION + 1;
    call.storage_size = sizeof(g_fs_delegate_transaction_storage);

    EXPECT_EQ(G_FS_DELEGATE_UNSUPPORTED, filesystemDelegateRegister(&server, &call));
}

TEST_F(FilesystemTaskDelegate, UnwindsEveryMappingFailure) {
    for(int phase = 0; phase < 5; ++phase) {
        SCOPED_TRACE(phase);
        failFrame = phase < 2 ? phase : -1;
        failMap = phase == 2 || phase == 3 ? phase - 2 : -1;
        failRange = phase == 4;

        EXPECT_EQ(G_FS_DELEGATE_NO_MEMORY, registerMount());
        EXPECT_TRUE(frames.empty());
        EXPECT_TRUE(serverRanges.ranges.empty());
        EXPECT_EQ(0U, filesystemDelegateRecordCount);
    }
}

TEST_F(FilesystemTaskDelegate, PublishesCanonicalLookupNodesOnlyAtSuccessfulCompletion) {
    ASSERT_EQ(G_FS_DELEGATE_SUCCESSFUL, registerMount());

    g_fs_virt_id found = G_FS_VIRT_ID_NONE;
    respond([&] {
        EXPECT_EQ(G_FS_DELEGATE_REQUEST_LOOKUP, notification.type);
        EXPECT_GT(notification.transaction, UINT32_MAX);
        EXPECT_STREQ("HELLO.TXT", shared()->data.lookup.name);
        EXPECT_EQ(G_FS_DELEGATE_BUSY, filesystemDelegateUnregister(&server, mount));

        g_syscall_fs_delegate_node_create call{};
        call.transaction = notification.transaction; call.parent_id = mount;
        call.name = "hello.txt"; call.type = G_FS_NODE_TYPE_FILE; call.phys_fs_id = UINT64_C(0x400000007);

        EXPECT_EQ(G_FS_DELEGATE_NOT_OWNER, filesystemDelegateCreateNode(&stranger, &call));
        ASSERT_EQ(G_FS_DELEGATE_SUCCESSFUL, filesystemDelegateCreateNode(&server, &call));

        found = call.created_id;

        EXPECT_EQ(nullptr, filesystemGetNode(found));

        shared()->data.lookup.result_node_id = found;
        shared()->data.lookup.result_status = G_FS_LOOKUP_SUCCESSFUL;

        EXPECT_EQ(G_FS_DELEGATE_SUCCESSFUL, filesystemDelegateComplete(&server, notification.transaction));
    });
    g_fs_node* node = nullptr;

    EXPECT_EQ(G_FS_OPEN_SUCCESSFUL, root()->delegate->lookup(root(), "HELLO.TXT", &node));
    ASSERT_NE(nullptr, node);
    EXPECT_EQ(found, node->id);
    EXPECT_EQ(UINT64_C(0x400000007), node->physicalId);
    EXPECT_STREQ("hello.txt", node->name);
}

TEST_F(FilesystemTaskDelegate, FailedDirectoryRefreshRollsBackAllStagedNodes) {
    ASSERT_EQ(G_FS_DELEGATE_SUCCESSFUL, registerMount());

    auto count = filesystemDelegateNodeCount;
    respond([&] {
        g_syscall_fs_delegate_node_create call{};
        call.transaction = notification.transaction; call.parent_id = mount;
        call.name = "uncommitted"; call.type = G_FS_NODE_TYPE_FOLDER;

        ASSERT_EQ(G_FS_DELEGATE_SUCCESSFUL, filesystemDelegateCreateNode(&server, &call));

        shared()->data.directory_refresh.result_status = G_FS_DIRECTORY_REFRESH_ERROR;

        EXPECT_EQ(G_FS_DELEGATE_SUCCESSFUL, filesystemDelegateComplete(&server, notification.transaction));
    });

    EXPECT_EQ(G_FS_DIRECTORY_REFRESH_ERROR, root()->delegate->refreshDir(root()));
    EXPECT_EQ(nullptr, root()->children);
    EXPECT_EQ(count, filesystemDelegateNodeCount);
}

TEST_F(FilesystemTaskDelegate, ValidatesHeadersOwnersReadCountsAndRequestPointersBeforeCompletion) {
    ASSERT_EQ(G_FS_DELEGATE_SUCCESSFUL, registerMount());

    std::array<uint8_t, 512> buffer{};
    delegateTestMap(&client, buffer.data(), buffer.size());
    respond([&] {
        auto expected = *shared();

        EXPECT_EQ(G_FS_DELEGATE_NOT_OWNER, filesystemDelegateComplete(&stranger, notification.transaction));

        shared()->header.version++;

        EXPECT_EQ(G_FS_DELEGATE_INVALID_RESULT, filesystemDelegateComplete(&server, notification.transaction));
        *shared() = expected;
        shared()->data.read.mapped_buffer = reinterpret_cast<void*>(0x400000);

        EXPECT_EQ(G_FS_DELEGATE_INVALID_RESULT, filesystemDelegateComplete(&server, notification.transaction));
        *shared() = expected;
        shared()->data.read.result_status = G_FS_READ_SUCCESSFUL;
        shared()->data.read.result_read = 513;

        EXPECT_EQ(G_FS_DELEGATE_INVALID_RESULT, filesystemDelegateComplete(&server, notification.transaction));

        std::memset(io(), 0x9a, 512);
        shared()->data.read.result_read = 512;

        EXPECT_EQ(G_FS_DELEGATE_SUCCESSFUL, filesystemDelegateComplete(&server, notification.transaction));
    });
    int64_t read = -1;

    EXPECT_EQ(G_FS_READ_SUCCESSFUL, root()->delegate->read(root(), buffer.data(), 7, 512, &read));
    EXPECT_EQ(512, read);
    EXPECT_TRUE(std::all_of(buffer.begin(), buffer.end(), [](uint8_t byte) { return byte == 0x9a; }));
}

TEST_F(FilesystemTaskDelegate, BoundsTransfersAndSeparatesCallerMemoryFromTheDriverWindow) {
    ASSERT_EQ(G_FS_DELEGATE_SUCCESSFUL, registerMount());

    std::vector<uint8_t> buffer(8192, 0x61);
    delegateTestMap(&client, buffer.data(), buffer.size());
    respond([&] {
        EXPECT_EQ(4096U, shared()->data.write.length);
        EXPECT_NE(buffer.data(), shared()->data.write.mapped_buffer);
        EXPECT_TRUE(std::all_of(io(), io() + 4096, [](uint8_t value) { return value == 0x61; }));

        shared()->data.write.result_status = G_FS_WRITE_SUCCESSFUL;
        shared()->data.write.result_write = 4096;

        EXPECT_EQ(G_FS_DELEGATE_SUCCESSFUL, filesystemDelegateComplete(&server, notification.transaction));
    });
    int64_t written = -1;

    EXPECT_EQ(G_FS_WRITE_SUCCESSFUL, root()->delegate->write(root(), buffer.data(), 19, buffer.size(), &written));
    EXPECT_EQ(4096, written);
}

TEST_F(FilesystemTaskDelegate, TimeoutRetainsItsWindowUntilAcknowledgementAndDiscardsLateWrites) {
    ASSERT_EQ(G_FS_DELEGATE_SUCCESSFUL, registerMount());

    std::array<uint8_t, 32> buffer{};
    delegateTestMap(&client, buffer.data(), buffer.size());
    select(&client); serve = {};
    int64_t read = 0;

    EXPECT_EQ(G_FS_READ_ERROR, root()->delegate->read(root(), buffer.data(), 0, 32, &read));
    EXPECT_EQ(-1, read);
    EXPECT_GE(delegateClock.time, G_FS_DELEGATE_TIMEOUT_MS);
    EXPECT_EQ(2U, frames.size());

    select(&server);

    EXPECT_EQ(G_FS_DELEGATE_BUSY, filesystemDelegateUnregister(&server, mount));

    std::memset(io(), 0xfc, 32);

    EXPECT_EQ(G_FS_DELEGATE_STALE_TRANSACTION, filesystemDelegateComplete(&server, notification.transaction));
    EXPECT_TRUE(std::all_of(buffer.begin(), buffer.end(), [](uint8_t value) { return value == 0; }));
    EXPECT_EQ(G_FS_DELEGATE_SUCCESSFUL, filesystemDelegateUnregister(&server, mount));
    EXPECT_TRUE(frames.empty());
}

TEST_F(FilesystemTaskDelegate, TerminatingCallerUnwindsBeforeItsTransactionCanBeReused) {
    ASSERT_EQ(G_FS_DELEGATE_SUCCESSFUL, registerMount());

    respond([&] { client.terminationRequested = true; });
    uint64_t length = 0;

    EXPECT_EQ(G_FS_LENGTH_ERROR, root()->delegate->getLength(root(), &length));
    EXPECT_NE(nullptr, filesystemDelegates->transaction);
    EXPECT_EQ(nullptr, filesystemDelegates->transaction->buffer);

    select(&server);

    EXPECT_EQ(G_FS_DELEGATE_STALE_TRANSACTION, filesystemDelegateComplete(&server, notification.transaction));
    EXPECT_EQ(nullptr, filesystemDelegates->transaction);
}

TEST_F(FilesystemTaskDelegate, ServingTaskExitDetachesAndReleasesPagesWhileWakingTheCaller) {
    ASSERT_EQ(G_FS_DELEGATE_SUCCESSFUL, registerMount());

    auto old = root();
    respond([&] { server.alive = false; filesystemDelegateTaskRemoved(&server); });

    EXPECT_EQ(G_FS_OPEN_ERROR, old->delegate->open(old, G_FILE_FLAG_MODE_READ));
    EXPECT_TRUE(frames.empty());
    EXPECT_FALSE(filesystemFindExistingChild(mountFolder, "fat0"));
    EXPECT_FALSE(filesystemDelegateAvailable(old));
}

TEST_F(FilesystemTaskDelegate, FullMessageQueueLeavesNoTransactionOrPinnedCaller) {
    ASSERT_EQ(G_FS_DELEGATE_SUCCESSFUL, registerMount());

    failSend = true; select(&client);
    uint64_t length = 0;

    EXPECT_EQ(G_FS_LENGTH_ERROR, root()->delegate->getLength(root(), &length));
    EXPECT_EQ(nullptr, filesystemDelegates->transaction);
    EXPECT_EQ(0U, server.references);
}

TEST_F(FilesystemTaskDelegate, RejectsSelfRecursionAndReadOnlyOrUnmappedUserAddresses) {
    ASSERT_EQ(G_FS_DELEGATE_SUCCESSFUL, registerMount());

    uint64_t length = 0;

    EXPECT_EQ(G_FS_LENGTH_ERROR, root()->delegate->getLength(root(), &length));

    uint8_t buffer = 0;

    EXPECT_FALSE(memoryCopyFromUser(&server, &buffer, 0xffff800000000000ULL, 1));
    EXPECT_FALSE(memoryCopyToUser(&server, 0x1000, &buffer, 1));

    delegateTestMap(&server, &buffer, 1);
    spaces[currentSpace][uintptr_t(&buffer) & ~uintptr_t(G_PAGE_ALIGN_MASK)] &= ~G_PAGE_WRITABLE_FLAG;

    EXPECT_FALSE(memoryCopyToUser(&server, uintptr_t(&buffer), &buffer, 1));
    EXPECT_TRUE(memoryCopyFromUser(&server, &buffer, uintptr_t(&buffer), 1));
}

TEST_F(FilesystemTaskDelegate, SyscallsEnforcePrivilegeAndRejectInvalidCallerPointers) {
    char name[] = "public";
    g_syscall_fs_delegate_register call{};
    call.name = name; call.version = G_FS_DELEGATE_VERSION;
    call.storage_size = sizeof(g_fs_delegate_transaction_storage);
    delegateTestMap(&stranger, &call, sizeof(call));
    delegateTestMap(&stranger, name, sizeof(name));
    select(&stranger); stranger.securityLevel = G_SECURITY_LEVEL_APPLICATION;
    syscallTestInvoke(&stranger, (g_syscall_handler) syscallFsDelegateRegister, &call, sizeof(g_syscall_fs_delegate_register));

    EXPECT_EQ(G_FS_DELEGATE_NOT_OWNER, g_fs_delegate_status(call.result));
    EXPECT_EQ(0U, filesystemDelegateRecordCount);

    syscallTestInvoke(&stranger, (g_syscall_handler) syscallFsDelegateRegister, reinterpret_cast<g_syscall_fs_delegate_register*>(0x1000), sizeof(g_syscall_fs_delegate_register));

    EXPECT_EQ(0U, filesystemDelegateRecordCount);
}

TEST_F(FilesystemTaskDelegate, IoSyscallsRejectNegativeLengthsAndKernelBuffers) {
    select(&client);
    g_syscall_fs_read read{};
    g_syscall_fs_write write{};
    delegateTestMap(&client, &read, sizeof(read));
    delegateTestMap(&client, &write, sizeof(write));
    for(auto length : {INT64_C(-1), INT64_C(1) << 32, INT64_C(1)}) {
        SCOPED_TRACE(length);
        read.length = write.length = length;
        read.buffer = reinterpret_cast<uint8_t*>(0xffff800000000000ULL);
        write.buffer = reinterpret_cast<uint8_t*>(0xffff800000000000ULL);
        if(length != 1) {
            read.buffer = reinterpret_cast<uint8_t*>(0x1000);
            write.buffer = reinterpret_cast<uint8_t*>(0x1000);
        }

        syscallTestInvoke(&client, (g_syscall_handler) syscallFsRead, &read, sizeof(g_syscall_fs_read));
        syscallTestInvoke(&client, (g_syscall_handler) syscallFsWrite, &write, sizeof(g_syscall_fs_write));

        EXPECT_EQ(G_FS_READ_ERROR, g_fs_read_status(read.status));
        EXPECT_EQ(G_FS_WRITE_ERROR, g_fs_write_status(write.status));
        EXPECT_EQ(-1, int64_t(read.result));
        EXPECT_EQ(-1, int64_t(write.result));
    }
}

TEST_F(FilesystemTaskDelegate, CreatesTypedNodesOnlyWhenTheDriverCommitsTheRequest)
{
    ASSERT_EQ(G_FS_DELEGATE_SUCCESSFUL, registerMount());

    root()->upToDate = true;
    respond([&] {
        EXPECT_EQ(G_FS_DELEGATE_REQUEST_CREATE, notification.type);
        EXPECT_EQ(G_FS_NODE_TYPE_FOLDER, shared()->data.create.type);
        EXPECT_STREQ("new folder", shared()->data.create.name);

        g_syscall_fs_delegate_node_create call{};
        call.transaction = notification.transaction; call.parent_id = mount;
        call.name = "new folder"; call.type = G_FS_NODE_TYPE_FILE; call.phys_fs_id = 71;

        EXPECT_EQ(G_FS_DELEGATE_INVALID_ARGUMENT, filesystemDelegateCreateNode(&server, &call));

        call.type = G_FS_NODE_TYPE_FOLDER;

        ASSERT_EQ(G_FS_DELEGATE_SUCCESSFUL, filesystemDelegateCreateNode(&server, &call));
        EXPECT_FALSE(filesystemFindExistingChild(root(), "new folder"));

        shared()->data.create.result_node_id = call.created_id;
        shared()->data.create.result_status = G_FS_MUTATION_SUCCESSFUL;

        EXPECT_EQ(G_FS_DELEGATE_SUCCESSFUL, filesystemDelegateComplete(&server, notification.transaction));
    });
    g_fs_node* child = nullptr;

    EXPECT_EQ(G_FS_MUTATION_SUCCESSFUL, root()->delegate->makeDirectory(root(), "new folder", &child));
    ASSERT_NE(nullptr, child);
    EXPECT_EQ(71U, child->physicalId);
    EXPECT_TRUE(filesystemFindExistingChild(root(), "new folder"));
    EXPECT_FALSE(root()->upToDate);
}

TEST_F(FilesystemTaskDelegate, FailedCreatesRollBackStagedNodes)
{
    ASSERT_EQ(G_FS_DELEGATE_SUCCESSFUL, registerMount());

    respond([&] {
        g_syscall_fs_delegate_node_create call{};
        call.transaction = notification.transaction; call.parent_id = mount;
        call.name = "new.txt"; call.type = G_FS_NODE_TYPE_FILE; call.phys_fs_id = 71;

        ASSERT_EQ(G_FS_DELEGATE_SUCCESSFUL, filesystemDelegateCreateNode(&server, &call));

        shared()->data.create.result_status = G_FS_MUTATION_NO_SPACE;

        EXPECT_EQ(G_FS_DELEGATE_SUCCESSFUL, filesystemDelegateComplete(&server, notification.transaction));
    });
    g_fs_node* child = nullptr;

    EXPECT_EQ(G_FS_OPEN_ERROR, root()->delegate->create(root(), "new.txt", &child));
    EXPECT_EQ(nullptr, child);
    EXPECT_EQ(nullptr, root()->children);
}

TEST_F(FilesystemTaskDelegate, RemovalDetachesTheNameAndPreservesTheOldDescriptorIdentity)
{
    ASSERT_EQ(G_FS_DELEGATE_SUCCESSFUL, registerMount());

    auto node = filesystemCreateDetachedNode(G_FS_NODE_TYPE_FILE, "old.txt");
    node->parent = root(); node->delegate = root()->delegate; node->physicalId = 71;
    auto entry = static_cast<g_fs_node_entry*>(heapAllocate(sizeof(g_fs_node_entry)));
    entry->node = node; entry->next = nullptr;
    filesystemPublishNode(node, entry);
    respond([&] {
        EXPECT_EQ(G_FS_DELEGATE_REQUEST_REMOVE, notification.type);

        shared()->data.remove.result_status = G_FS_MUTATION_NOT_EMPTY;

        EXPECT_EQ(G_FS_DELEGATE_SUCCESSFUL, filesystemDelegateComplete(&server, notification.transaction));
    });

    EXPECT_EQ(G_FS_MUTATION_NOT_EMPTY, node->delegate->mutate(node, G_FS_DELEGATE_REQUEST_REMOVE, 0));
    EXPECT_TRUE(filesystemFindExistingChild(root(), "old.txt"));

    respond([&] {
        shared()->data.remove.result_status = G_FS_MUTATION_SUCCESSFUL;

        EXPECT_EQ(G_FS_DELEGATE_SUCCESSFUL, filesystemDelegateComplete(&server, notification.transaction));
    });

    EXPECT_EQ(G_FS_MUTATION_SUCCESSFUL, node->delegate->mutate(node, G_FS_DELEGATE_REQUEST_REMOVE, 0));
    EXPECT_FALSE(filesystemFindExistingChild(root(), "old.txt"));
    EXPECT_TRUE(node->removed);
    EXPECT_EQ(node, filesystemGetNode(node->id));

    respond([&] {
        EXPECT_EQ(G_FS_DELEGATE_REQUEST_RESIZE, notification.type);
        EXPECT_EQ(71U, shared()->data.resize.phys_fs_id);
        EXPECT_EQ(UINT64_C(0x100000001), shared()->data.resize.length);

        shared()->data.resize.result_status = G_FS_MUTATION_SUCCESSFUL;

        EXPECT_EQ(G_FS_DELEGATE_SUCCESSFUL, filesystemDelegateComplete(&server, notification.transaction));
    });

    EXPECT_EQ(G_FS_MUTATION_SUCCESSFUL, node->delegate->mutate(node, G_FS_DELEGATE_REQUEST_RESIZE, UINT64_C(0x100000001)));
}

static std::string mutationPath;
static g_fd mutationFd;
static uint64_t mutationLength;
static g_fs_delegate_request_type mutationType;
g_fs_mutation_status filesystemMakeDirectory(g_task*, const char* path)
{
    mutationPath = path;
    return G_FS_MUTATION_NO_SPACE;
}

g_fs_mutation_status filesystemRemove(g_task*, const char* path, bool directory)
{
    mutationPath = path;
    return directory ? G_FS_MUTATION_NOT_EMPTY : G_FS_MUTATION_SUCCESSFUL;
}

g_fs_mutation_status filesystemMutateDescriptor(g_task*, g_fd fd, g_fs_delegate_request_type type, uint64_t length)
{
    mutationFd = fd; mutationType = type; mutationLength = length;
    return G_FS_MUTATION_SUCCESSFUL;
}
g_fs_mutation_status filesystemRename(g_task*, const char* source, const char* destination, bool replace)
{
    mutationPath = std::string(source) + " -> " + destination;
    return replace ? G_FS_MUTATION_SUCCESSFUL : G_FS_MUTATION_ALREADY_EXISTS;
}
#include "production/filesystem_mutation_syscalls.inc"

TEST_F(FilesystemTaskDelegate, MutationSyscallsSnapshotAndBoundUserInputs)
{
    select(&client);
    char path[] = "/mount/fat0/new///";
    g_syscall_fs_path_mutation call{path, G_FS_MUTATION_ERROR};
    delegateTestMap(&client, &call, sizeof(call));
    delegateTestMap(&client, path, sizeof(path));
    syscallTestInvoke(&client, (g_syscall_handler) syscallFsCreateDirectory, &call, sizeof(g_syscall_fs_path_mutation));

    EXPECT_EQ(G_FS_MUTATION_NO_SPACE, int(call.status));
    EXPECT_EQ("/mount/fat0/new", mutationPath);

    syscallTestInvoke(&client, (g_syscall_handler) syscallFsUnlinkDirectory, &call, sizeof(g_syscall_fs_path_mutation));

    EXPECT_EQ(G_FS_MUTATION_NOT_EMPTY, int(call.status));

    syscallTestInvoke(&client, (g_syscall_handler) syscallFsUnlink, &call, sizeof(g_syscall_fs_path_mutation));

    EXPECT_EQ(G_FS_MUTATION_SUCCESSFUL, int(call.status));

    call.path = reinterpret_cast<const char*>(0xffff800000000000ULL);
    syscallTestInvoke(&client, (g_syscall_handler) syscallFsCreateDirectory, &call, sizeof(g_syscall_fs_path_mutation));

    EXPECT_EQ(G_FS_MUTATION_INVALID_ARGUMENT, int(call.status));

    syscallTestInvoke(&client, (g_syscall_handler) syscallFsCreateDirectory, reinterpret_cast<g_syscall_fs_path_mutation*>(0x1000), sizeof(g_syscall_fs_path_mutation));
    std::array<char, G_PATH_MAX> unterminated;
    unterminated.fill('a');
    delegateTestMap(&client, unterminated.data(), unterminated.size());
    call.path = unterminated.data();
    syscallTestInvoke(&client, (g_syscall_handler) syscallFsCreateDirectory, &call, sizeof(g_syscall_fs_path_mutation));

    EXPECT_EQ(G_FS_MUTATION_INVALID_ARGUMENT, int(call.status));

    failHeap = 0;
    call.path = path;
    syscallTestInvoke(&client, (g_syscall_handler) syscallFsCreateDirectory, &call, sizeof(g_syscall_fs_path_mutation));

    EXPECT_EQ(G_FS_MUTATION_ERROR, int(call.status));

    failHeap = -1;
    g_syscall_fs_fd_mutation descriptor{42, UINT64_C(0x100000001), G_FS_MUTATION_ERROR};
    delegateTestMap(&client, &descriptor, sizeof(descriptor));
    syscallTestInvoke(&client, (g_syscall_handler) syscallFsTruncate, &descriptor, sizeof(g_syscall_fs_fd_mutation));

    EXPECT_EQ(G_FS_MUTATION_SUCCESSFUL, int(descriptor.status));
    EXPECT_EQ(42, mutationFd);
    EXPECT_EQ(UINT64_C(0x100000001), mutationLength);
    EXPECT_EQ(G_FS_DELEGATE_REQUEST_RESIZE, mutationType);

    syscallTestInvoke(&client, (g_syscall_handler) syscallFsSync, &descriptor, sizeof(g_syscall_fs_fd_mutation));

    EXPECT_EQ(G_FS_DELEGATE_REQUEST_FLUSH, mutationType);
}

TEST_F(FilesystemTaskDelegate, CheckedUserCopiesResolveUntouchedStackPages)
{
    select(&client);
    alignas(4096) std::array<uint8_t, 4096> page{};
    userStackPage = uintptr_t(page.data());
    std::array<uint8_t, 4096> source;
    source.fill(0x9a);

    EXPECT_TRUE(memoryCopyToUser(&client, userStackPage, source.data(), source.size()));
    EXPECT_EQ(source, page);

    spaces[currentSpace].erase(userStackPage);
    source.fill(0);

    EXPECT_TRUE(memoryCopyFromUser(&client, source.data(), userStackPage, source.size()));
    EXPECT_EQ(source, page);
}

TEST_F(FilesystemTaskDelegate, ShutdownFlushExcludesOtherTransactionsUntilCancellation)
{
    ASSERT_EQ(G_FS_DELEGATE_SUCCESSFUL, registerMount());

    respond([&] {
        EXPECT_EQ(G_FS_DELEGATE_REQUEST_FLUSH, notification.type);

        shared()->data.flush.result_status = G_FS_MUTATION_SUCCESSFUL;

        EXPECT_EQ(G_FS_DELEGATE_SUCCESSFUL, filesystemDelegateComplete(&server, notification.transaction));
    });

    EXPECT_TRUE(filesystemDelegateFlushAll(client.id));

    select(&stranger);
    auto sent = notifications;

    EXPECT_EQ(G_FS_MUTATION_ERROR, root()->delegate->mutate(root(), G_FS_DELEGATE_REQUEST_FLUSH, 0));
    EXPECT_EQ(sent, notifications);

    filesystemDelegateResume();
    select(&client);

    EXPECT_EQ(G_FS_MUTATION_SUCCESSFUL, root()->delegate->mutate(root(), G_FS_DELEGATE_REQUEST_FLUSH, 0));
}

TEST_F(FilesystemTaskDelegate, FailedShutdownFlushKeepsTheBarrierUntilExplicitResume)
{
    ASSERT_EQ(G_FS_DELEGATE_SUCCESSFUL, registerMount());

    respond([&] {
        shared()->data.flush.result_status = G_FS_MUTATION_ERROR;

        EXPECT_EQ(G_FS_DELEGATE_SUCCESSFUL, filesystemDelegateComplete(&server, notification.transaction));
    });

    EXPECT_FALSE(filesystemDelegateFlushAll(client.id));
    EXPECT_EQ(client.id, filesystemDelegateDrainOwner);

    filesystemDelegateResume();

    EXPECT_EQ(G_TID_NONE, filesystemDelegateDrainOwner);
}

TEST_F(FilesystemTaskDelegate, RenameSyscallCopiesBothPathsBeforeCallingTheFilesystem)
{
    select(&client);
    char source[] = "/mount/fat0/source";
    char destination[] = "/mount/fat0/destination";
    g_syscall_fs_rename call{source, destination, false, G_FS_MUTATION_ERROR};
    delegateTestMap(&client, &call, sizeof(call));
    delegateTestMap(&client, source, sizeof(source));
    delegateTestMap(&client, destination, sizeof(destination));
    syscallTestInvoke(&client, (g_syscall_handler) syscallFsRename, &call, sizeof(g_syscall_fs_rename));
    EXPECT_EQ(G_FS_MUTATION_ALREADY_EXISTS, int(call.status));
    EXPECT_EQ("/mount/fat0/source -> /mount/fat0/destination", mutationPath);
    call.replace = true;
    syscallTestInvoke(&client, (g_syscall_handler) syscallFsRename, &call, sizeof(g_syscall_fs_rename));
    EXPECT_EQ(G_FS_MUTATION_SUCCESSFUL, int(call.status));
    call.destination = nullptr;
    mutationPath.clear();
    syscallTestInvoke(&client, (g_syscall_handler) syscallFsRename, &call, sizeof(g_syscall_fs_rename));
    EXPECT_EQ(G_FS_MUTATION_INVALID_ARGUMENT, int(call.status));
    EXPECT_TRUE(mutationPath.empty());
}

TEST_F(FilesystemTaskDelegate, RenameDelegationValidatesItsInputsAndPreservesFullPhysicalIds)
{
    ASSERT_EQ(G_FS_DELEGATE_SUCCESSFUL, registerMount());
    select(&client);
    auto node = root();
    node->physicalId = UINT64_C(0x100000001);
    respond([&] {
        EXPECT_EQ(G_FS_DELEGATE_REQUEST_RENAME, shared()->header.type);
        EXPECT_EQ(UINT64_C(0x100000001), shared()->data.rename.phys_fs_id);
        EXPECT_STREQ("renamed", shared()->data.rename.name);
        EXPECT_EQ(UINT64_MAX, shared()->data.rename.replaced_phys_fs_id);
        shared()->data.rename.parent_phys_fs_id++;
        shared()->data.rename.result_status = G_FS_MUTATION_SUCCESSFUL;
        EXPECT_EQ(G_FS_DELEGATE_INVALID_RESULT, filesystemDelegateComplete(&server, notification.transaction));
        shared()->data.rename.parent_phys_fs_id--;
        EXPECT_EQ(G_FS_DELEGATE_SUCCESSFUL, filesystemDelegateComplete(&server, notification.transaction));
    });
    EXPECT_EQ(G_FS_MUTATION_SUCCESSFUL, node->delegate->rename(node, node, "renamed", nullptr));
}

static g_fs_open_status linkReadStatus = G_FS_OPEN_SUCCESSFUL;
g_fs_mutation_status filesystemSymlink(g_task*, const char* target, const char* path)
{
    mutationPath = std::string(path) + " -> " + target;
    return G_FS_MUTATION_SUCCESSFUL;
}
g_fs_open_status filesystemReadLinkPath(g_task*, const char*, char* target, uint32_t* length)
{
    strcpy(target, "abcdef"); *length = 6; return linkReadStatus;
}
#include "production/filesystem_link_syscalls.inc"

TEST_F(FilesystemTaskDelegate, ReadlinkTruncatesBytesWithoutWritingATerminatorAndPropagatesErrors)
{
    select(&client);
    char path[] = "/link.link", buffer[8]; memset(buffer, 'Z', sizeof(buffer));
    g_syscall_fs_readlink call{path, buffer, 3, -1, G_FS_OPEN_ERROR};
    delegateTestMap(&client, path, sizeof(path)); delegateTestMap(&client, buffer, sizeof(buffer));
    delegateTestMap(&client, &call, sizeof(call));
    linkReadStatus = G_FS_OPEN_SUCCESSFUL;
    syscallTestInvoke(&client, (g_syscall_handler) syscallFsReadlink, &call, sizeof(g_syscall_fs_readlink));
    EXPECT_EQ(3, call.result); EXPECT_EQ(0, memcmp(buffer, "abcZZZZZ", 8));
    linkReadStatus = G_FS_OPEN_LOOP;
    syscallTestInvoke(&client, (g_syscall_handler) syscallFsReadlink, &call, sizeof(g_syscall_fs_readlink));
    EXPECT_EQ(-1, call.result); EXPECT_EQ(G_FS_OPEN_LOOP, int(call.status));
    call.capacity = 0; syscallTestInvoke(&client, (g_syscall_handler) syscallFsReadlink, &call, sizeof(g_syscall_fs_readlink));
    EXPECT_EQ(-1, call.result); EXPECT_EQ(G_FS_OPEN_INVALID_LINK, int(call.status));
}

TEST_F(FilesystemTaskDelegate, SymlinkSnapshotsBothStringsAndPreservesTargetTrailingSlashes)
{
    select(&client);
    char target[] = "../folder/", path[] = "/link.link";
    g_syscall_fs_symlink call{target, path, G_FS_MUTATION_ERROR};
    delegateTestMap(&client, target, sizeof(target)); delegateTestMap(&client, path, sizeof(path));
    delegateTestMap(&client, &call, sizeof(call));
    syscallTestInvoke(&client, (g_syscall_handler) syscallFsSymlink, &call, sizeof(g_syscall_fs_symlink));
    EXPECT_EQ(G_FS_MUTATION_SUCCESSFUL, int(call.status));
    EXPECT_EQ("/link.link -> ../folder/", mutationPath);
}

TEST_F(FilesystemTaskDelegate, LinkSyscallsDistinguishOverlongAndInaccessiblePaths)
{
    select(&client);
    std::string path(G_PATH_MAX, 'x');
    char buffer[2]{};
    g_syscall_fs_readlink call{path.c_str(), buffer, 2, -1, G_FS_OPEN_ERROR};
    delegateTestMap(&client, path.data(), path.size());
    delegateTestMap(&client, buffer, sizeof(buffer));
    delegateTestMap(&client, &call, sizeof(call));
    syscallTestInvoke(&client, (g_syscall_handler) syscallFsReadlink, &call, sizeof(g_syscall_fs_readlink));
    EXPECT_EQ(-1, call.result);
    EXPECT_EQ(G_FS_OPEN_NAME_TOO_LONG, int(call.status));
    call.path = nullptr;
    syscallTestInvoke(&client, (g_syscall_handler) syscallFsReadlink, &call, sizeof(g_syscall_fs_readlink));
    EXPECT_EQ(-1, call.result);
    EXPECT_EQ(G_FS_OPEN_ERROR, int(call.status));
}
