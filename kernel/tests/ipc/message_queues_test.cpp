#include "support/fixture.hpp"

namespace
{
#include "support/tracked_heap.hpp"

using g_tid = int;
struct g_wait_queue { int count; };
void waitQueueInitialize(g_wait_queue* q) { q->count = 0; }
void waitQueueAdd(g_wait_queue* q, int) { ++q->count; }
void waitQueueRemove(g_wait_queue* q, int) { q->count = 0; }
int wakeCount;
void waitQueueWake(g_wait_queue* q) { if(q->count) ++wakeCount; q->count = 0; }
struct g_task { int id; } sender{1}, receiverTask{2};
bool alive;
int pins;
void* taskGlobalMap;
void mutexInitializeTask(g_mutex* m, const char* = nullptr) { m->depth = 0; }
g_task* taskingAcquireTask(int id) {
    if(!alive || id != 2) return nullptr;
    ++pins;
    return &receiverTask;
}
void taskingReleaseTask(g_task* task) { if(task) --pins; }
void taskingPrepareWait(g_task*, const char*) {}
void taskingWake(g_task*) {}
void memoryCopy(void* target, const void* source, size_t size) {
    EXPECT_TRUE(pins > 0); std::memcpy(target, source, size);
}
#include "production/message_types.inc"
#include "production/message_queue_layout.inc"
#include "production/hashmap_hpp.inc"
#include "production/message_queues_cpp.inc"

TEST(MessageQueues, RejectsDeadReceiversAndReclaimsQueuedMessagesAndWaiters) {
    taskGlobalMap = nullptr;
    messageQueuesInitialize();
    uint8_t payload[G_MESSAGE_MAXIMUM_MESSAGE_LENGTH]{};
    EXPECT_EQ(G_MESSAGE_SEND_STATUS_FAILED, messageQueueSend(1, 2, payload, 4, -1));
    EXPECT_EQ(0, messageQueues->itemCount);
    alive = true;
    auto baseline = heapBlocks.size();
    for(int round = 0; round < 100; ++round) {
        alive = true;
        while(messageQueueSend(1, 2, payload, sizeof(payload), -1) == G_MESSAGE_SEND_STATUS_SUCCESSFUL) {}
        EXPECT_EQ(0, pins);
        EXPECT_TRUE(messageQueueWaitForSend(&sender, 2, sizeof(payload)));
        alive = false;
        EXPECT_FALSE(messageQueueWaitForSend(&sender, 2, sizeof(payload)));
        messageQueueUnwaitForSend(1, 2);
        messageQueueTaskRemoved(2);
        EXPECT_TRUE(messageQueues->itemCount == 0 && wakeCount == round + 1);
        EXPECT_EQ(G_MESSAGE_SEND_STATUS_FAILED, messageQueueSend(1, 2, payload, 4, -1));
        messageQueueUnwaitForSend(1, 2);
        EXPECT_EQ(baseline, heapBlocks.size());
        EXPECT_EQ(0, pins);
    }
    hashmapDestroy(messageQueues);
    EXPECT_TRUE(heapBlocks.empty());
}
}
