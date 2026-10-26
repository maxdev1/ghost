#include "support/fixture.hpp"

namespace
{
#include "support/tracked_heap.hpp"

using g_tid = int;
struct g_interrupt_guard {};
struct g_task { int id; } current;
std::vector<int> woken;
g_task* taskingAcquireTask(int id) { current.id = id; return &current; }
void taskingReleaseTask(g_task*) {}
void taskingWake(g_task* task) { if(task) woken.push_back(task->id); }

#include "production/spinlock_host_hpp.inc"
#include "production/wait_queue_hpp.inc"
#include "production/wait_queue_cpp.inc"

TEST(WaitQueues, DeduplicatesRegistrationsAndRemovesAllWaitsForExitingTasks) {
    g_wait_queue queues[3];
    for(auto& queue : queues) waitQueueInitialize(&queue);
    for(auto& queue : queues) {
        waitQueueAdd(&queue, 7);
        waitQueueAdd(&queue, 7);
        waitQueueAdd(&queue, 8);
    }
    EXPECT_EQ(6, heapBlocks.size());
    waitQueueRemoveTask(7);
    EXPECT_EQ(3, heapBlocks.size());
    for(auto& queue : queues) waitQueueWake(&queue);
    EXPECT_TRUE(heapBlocks.empty() && !waitRegistry && woken.size() == 3);
    for(int id : woken) EXPECT_EQ(8, id);
    std::vector<std::thread> threads;
    for(int id = 0; id < 16; ++id)
        threads.emplace_back([&, id] {
            for(int round = 0; round < 200; ++round) {
                for(auto& queue : queues) waitQueueAdd(&queue, id);
                waitQueueRemoveTask(id);
            }
        });
    for(auto& thread : threads) thread.join();
    for(auto& queue : queues) EXPECT_TRUE(waitQueueIsEmpty(&queue));
    EXPECT_TRUE(heapBlocks.empty() && !waitRegistry);
}
}
