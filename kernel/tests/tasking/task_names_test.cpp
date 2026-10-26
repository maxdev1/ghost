#include "support/fixture.hpp"
#include <thread>

namespace
{
#include "support/tracked_heap.hpp"

using g_tid = int;
std::recursive_mutex registryMutex;
void acquireRegistry(g_mutex* lock) { registryMutex.lock(); ++lock->depth; }
void releaseRegistry(g_mutex* lock) { --lock->depth; registryMutex.unlock(); }
#define mutexAcquire acquireRegistry
#define mutexRelease releaseRegistry
#include "production/hashmap_hpp.inc"

g_mutex entryLock;
g_hashmap<int, const char*>* reverseMap;

#include "production/taskingDirectoryCopyIdentifier.inc"

TEST(TaskNames, CopiesBoundedSnapshotsThatSurviveRenameAndSourceDestruction) {
    reverseMap = hashmapCreateNumeric<int, const char*>(8);
    auto name = stringDuplicate("abcdefgh");
    hashmapPut(reverseMap, 7, (const char*)name);
    char out[8]; std::memset(out, '!', sizeof(out));
    taskingDirectoryCopyIdentifier(7, out, 4);
    EXPECT_TRUE(std::strcmp(out, "abc") == 0 && out[4] == '!');
    taskingDirectoryCopyIdentifier(7, out, 1); EXPECT_EQ(0, out[0]);
    out[0] = '!'; taskingDirectoryCopyIdentifier(7, out, 0); EXPECT_EQ('!', out[0]);
    taskingDirectoryCopyIdentifier(8, out, sizeof(out)); EXPECT_EQ(0, out[0]);
    std::thread writer([&] {
        for(int i = 0; i < 1000; ++i) {
            mutexAcquire(&entryLock);
            auto old = hashmapGet(reverseMap, 7, (const char*)nullptr);
            hashmapPut(reverseMap, 7, (const char*)stringDuplicate(i % 2 ? "abcdefgh" : "abcdefg"));
            heapFree((void*)old);
            mutexRelease(&entryLock);
        }
    });
    for(int i = 0; i < 1000; ++i) {
        taskingDirectoryCopyIdentifier(7, out, sizeof(out));
        EXPECT_EQ(0, std::strcmp(out, "abcdefg"));
    }
    writer.join();
    heapFree((void*)hashmapGet(reverseMap, 7, (const char*)nullptr));
    EXPECT_EQ(0, std::strcmp(out, "abcdefg")); // copy survives source teardown
    hashmapDestroy(reverseMap); EXPECT_TRUE(heapBlocks.empty());
}
}
