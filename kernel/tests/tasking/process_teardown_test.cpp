#include "support/fixture.hpp"

namespace
{
#include "support/tracked_heap.hpp"

using g_pid = int;
struct g_task {};
struct g_task_entry;
struct g_elf_object {};
struct g_process_info {};
struct g_process_spawn_arguments {};
struct g_memory_file_ondemand { g_memory_file_ondemand* next; };
struct g_address_range_pool {};
#include "production/process_layout.inc"

int callbacks;
void userMutexProcessRemoved(int pid) { EXPECT_EQ(42, pid); ++callbacks; }
void elfObjectDestroy(g_elf_object* p) { heapFree(p); ++callbacks; }
void filesystemProcessRemove(int pid) { EXPECT_EQ(42, pid); ++callbacks; }
void taskingMemoryDestroyPageSpace(uintptr_t p) { EXPECT_EQ(0x1000, p); ++callbacks; }
void addressRangePoolDestroy(g_address_range_pool*) { ++callbacks; }

#include "production/taskingDestroyProcess.inc"

TEST(ProcessTeardown, ReleasesProcessMetadataAndInvokesResourceOwners) {
    for(int round = 0; round < 100; ++round) {
        auto process = static_cast<g_process*>(heapAllocateClear(sizeof(g_process)));
        process->id = 42;
        process->pageSpace = 0x1000;
        process->object = static_cast<g_elf_object*>(heapAllocate(sizeof(g_elf_object)));
        process->virtualRangePool = static_cast<g_address_range_pool*>(heapAllocate(sizeof(g_address_range_pool)));
        process->environment.arguments = stringDuplicate("arguments");
        process->environment.executablePath = stringDuplicate("/binary");
        process->environment.workingDirectory = stringDuplicate("/");
        process->spawnArgs = static_cast<g_process_spawn_arguments*>(heapAllocate(sizeof(g_process_spawn_arguments)));
        process->tlsMaster.location = 0x5000;
        process->userProcessInfo = reinterpret_cast<g_process_info*>(0x6000);
        for(int i = 0; i < 5; ++i) {
            auto mapping = static_cast<g_memory_file_ondemand*>(heapAllocate(sizeof(g_memory_file_ondemand)));
            mapping->next = process->onDemandMappings;
            process->onDemandMappings = mapping;
        }
        callbacks = 0;
        taskingDestroyProcess(process);
        EXPECT_TRUE(callbacks == 5 && heapBlocks.empty());
    }
}
}
