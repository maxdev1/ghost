#include "support/fixture.hpp"

namespace
{
#include "support/tracked_heap.hpp"

constexpr int G_TASK_STATUS_DEAD = 3;
struct g_process;
struct g_task {
    g_process* process;
    int id, status;
    g_mutex lock;
    void* vm86Data;
    void* allocation;
    int waitersJoin;
};
struct g_task_entry { g_task* task; g_task_entry* next; };
struct g_process { g_mutex lock; g_task* main; g_task_entry* tasks; bool terminating; uintptr_t pageSpace; };
std::recursive_mutex cleanupMutex;
void reviewAcquire(g_mutex* m) { cleanupMutex.lock(); ++m->depth; }
void reviewRelease(g_mutex* m) { EXPECT_TRUE(m->depth > 0); --m->depth; cleanupMutex.unlock(); }
#define mutexAcquire reviewAcquire
#define mutexRelease reviewRelease
g_mutex taskingCleanupLock;
thread_local uintptr_t currentSpace = 0x444000;
int destroyedProcesses;
std::atomic<int> activeCleanup{0};
bool taskingTryRetireTask(g_task*) { return true; }
void taskingYield() { std::abort(); }
void taskingTerminateProcess(g_process* p) { p->terminating = true; }
void taskingCancelPendingSpawns(int) {}
void clockUnwaitForTime(g_task*) {}
void requestsTaskRemoved(g_task*) {}
void waitQueueRemoveTask(int) {}
void waitQueueWake(int*) {}
void taskingDirectoryTaskRemoved(int) {}
void userMutexTaskRemoved(int) {}
void messageQueueTaskRemoved(int) {}
uintptr_t taskingMemoryTemporarySwitchTo(uintptr_t p) { auto old = currentSpace; currentSpace = p; return old; }
void taskingMemoryTemporarySwitchBack(uintptr_t p) { currentSpace = p; }
void taskingMemoryDestroy(g_task* task) {
    EXPECT_TRUE(task->lock.depth == 0 && currentSpace == task->process->pageSpace);
    EXPECT_EQ(0, activeCleanup.fetch_add(1));
    for(int i = 0; i < 20; ++i) std::this_thread::yield();
    heapFree(task->allocation); task->allocation = nullptr;
    EXPECT_EQ(1, activeCleanup.fetch_sub(1));
}
void taskingDestroyProcess(g_process* p) {
    EXPECT_TRUE(!p->tasks && currentSpace == 0x444000);
    ++destroyedProcesses; heapFree(p);
}
#include "production/taskingProcessRemoveFromTaskList.inc"
#include "production/taskingDestroyTask.inc"

void run(bool mainFirst, bool concurrent) {
    auto process = static_cast<g_process*>(heapAllocateClear(sizeof(g_process)));
    auto main = static_cast<g_task*>(heapAllocateClear(sizeof(g_task)));
    auto sibling = static_cast<g_task*>(heapAllocateClear(sizeof(g_task)));
    process->main = main; process->pageSpace = 0x1000;
    main->process = sibling->process = process;
    main->id = 7; sibling->id = 8;
    main->status = sibling->status = G_TASK_STATUS_DEAD;
    main->allocation = heapAllocate(128); sibling->allocation = heapAllocate(128);
    auto first = static_cast<g_task_entry*>(heapAllocate(sizeof(g_task_entry)));
    auto last = static_cast<g_task_entry*>(heapAllocate(sizeof(g_task_entry)));
    *first = {main, last}; *last = {sibling, nullptr}; process->tasks = first;
    if(concurrent) {
        std::thread a([&] { taskingDestroyTask(main); });
        std::thread b([&] { taskingDestroyTask(sibling); });
        a.join(); b.join();
    } else if(mainFirst) {
        taskingDestroyTask(main);
        EXPECT_TRUE(process->terminating && process->tasks && process->main == main);
        EXPECT_TRUE(heapBlocks.count(main) && main->id == 7 && !main->allocation);
        taskingDestroyTask(sibling);
    } else {
        taskingDestroyTask(sibling);
        EXPECT_TRUE(!process->terminating && process->tasks && heapBlocks.count(main));
        taskingDestroyTask(main);
    }
    EXPECT_TRUE(heapBlocks.empty());
}
TEST(TaskTeardown, ConcurrentReapersFinalizeProcessOnceAndRetainMainMetadata) {
    for(int round = 0; round < 50; ++round) {
        run(true, false);
        run(false, false);
        run(false, true);
    }
    EXPECT_TRUE(destroyedProcesses == 150 && activeCleanup == 0);
}
}
