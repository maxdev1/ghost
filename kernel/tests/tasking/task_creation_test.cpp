#include "support/fixture.hpp"

namespace
{
#include "support/tracked_heap.hpp"

using g_security_level = int;
constexpr int G_TASK_TYPE_DEFAULT = 0;
struct g_process { g_mutex lock; bool terminating; uintptr_t pageSpace; };
struct g_task {
    g_process* process; int type, id;
    struct { uintptr_t start, end; } interruptStack, stack;
    struct state_t { uintptr_t rip, rsp, cs, ss, rflags; } *state;
};
int destroyed, published, stateResets, listed, initialized;
uintptr_t currentSpace = 0x1000;
int taskGlobalMap;
void _taskingInitializeTask(g_task* task, g_process* process, int) { ++initialized; task->process = process; }
uintptr_t taskingMemoryTemporarySwitchTo(uintptr_t space) { auto old = currentSpace; currentSpace = space; return old; }
void taskingMemoryTemporarySwitchBack(uintptr_t space) { currentSpace = space; }
bool taskingMemoryInitialize(g_task*) { return false; }
void taskingMemoryDestroy(g_task*) { ++destroyed; EXPECT_EQ(0x2000, currentSpace); }
void taskingStateReset(g_task*, uintptr_t, int) { ++stateResets; }
void taskingProcessAddToTaskList(g_process*, g_task*) { ++listed; }
void hashmapPut(int, int, g_task*) { ++published; }

#include "production/taskingCreateTask.inc"

class TaskCreation : public testing::Test
{
protected:
    void SetUp() override
    {
        destroyed = published = stateResets = listed = initialized = 0;
        currentSpace = 0x1000;
        ASSERT_TRUE(heapBlocks.empty());
    }
};

TEST_F(TaskCreation, RollsBackFailedMemoryBeforePublishingTask)
{

    g_process process{}; process.pageSpace = 0x2000;
    EXPECT_FALSE(taskingCreateTask(0x4000, &process, 2));
    EXPECT_TRUE(destroyed == 1 && !published && !stateResets && !listed && heapBlocks.empty());
    EXPECT_TRUE(currentSpace == 0x1000 && process.lock.depth == 0);
    EXPECT_EQ(1, initialized);
}

TEST_F(TaskCreation, RejectsTerminatingProcessBeforeInitializingTask)
{
    g_process process{};
    process.pageSpace = 0x2000;
    process.terminating = true;
    EXPECT_EQ(nullptr, taskingCreateTask(0x4000, &process, 2));
    EXPECT_EQ(0, initialized);
    EXPECT_EQ(0, destroyed);
    EXPECT_EQ(0, published);
    EXPECT_EQ(0, listed);
    EXPECT_TRUE(heapBlocks.empty());
    EXPECT_EQ(0, process.lock.depth);
}
}
