#include "support/fixture.hpp"

namespace
{
#include "support/tracked_heap.hpp"
#include "production/hashmap_hpp.inc"

using g_pid = int;
using g_fd = int;
constexpr int G_FD_NONE = -1;
struct g_file_descriptor { int id, references, waiters; bool closed; };
g_mutex descriptorLock;
int closes;
void waitQueueWake(int*) {}
void filesystemProcessReleaseDescriptor(g_file_descriptor* d) {
    EXPECT_TRUE(d->closed && d->references == 1); ++closes; heapFree(d);
}
void filesystemProcessCloseDescriptor(int, int) {}

struct g_filesystem_process { g_hashmap<int, g_file_descriptor*>* descriptors; };
g_hashmap<int, g_filesystem_process*>* filesystemProcessInfo;
void filesystemProcessRemoveDescriptor(int, int);
void filesystemClose(int pid, int fd, bool remove) {
    EXPECT_TRUE(pid == 42 && !remove && fd >= 3 && fd < 13);
    auto info = hashmapGet(filesystemProcessInfo, pid, static_cast<g_filesystem_process*>(nullptr));
    EXPECT_TRUE(info && info->descriptors->lock.depth == 0);
    EXPECT_TRUE(hashmapGet(info->descriptors, fd, static_cast<g_file_descriptor*>(nullptr)));
    ++closes;
}

#include "production/descriptor_map_teardown.inc"

TEST(DescriptorCleanup, ClosesEveryDescriptorAndReclaimsProcessMap) {
    filesystemProcessInfo = hashmapCreateNumeric<int, g_filesystem_process*>(16);
    for(int round = 0; round < 100; ++round) {
        auto info = static_cast<g_filesystem_process*>(heapAllocate(sizeof(g_filesystem_process)));
        info->descriptors = hashmapCreateNumeric<int, g_file_descriptor*>(16);
        hashmapPut(filesystemProcessInfo, 42, info);
        for(int fd = 3; fd < 13; ++fd) {
            auto descriptor = static_cast<g_file_descriptor*>(heapAllocateClear(sizeof(g_file_descriptor)));
            descriptor->id = fd;
            hashmapPut(info->descriptors, fd, descriptor);
        }
        closes = 0;
        filesystemProcessRemove(42);
        EXPECT_TRUE(closes == 10 && hashmapSize(filesystemProcessInfo) == 0);
        EXPECT_EQ(2, heapBlocks.size()); // persistent process-index map and buckets only
    }
    filesystemProcessRemove(42);
    hashmapDestroy(filesystemProcessInfo);
    EXPECT_TRUE(heapBlocks.empty());
}
}
