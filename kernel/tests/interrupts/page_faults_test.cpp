#include "support/fixture.hpp"

namespace
{
constexpr int G_TASK_TYPE_VITAL = 1, G_SECURITY_LEVEL_KERNEL = 0, G_TASK_STATUS_DEAD = 3;
struct g_task { int id, type, securityLevel, status; };
struct g_processor_state { uint64_t error, rip; };
int growth, dumped, scheduled;
uintptr_t exceptionsGetCR2() { return 0x1000; }
uintptr_t pagingVirtualToPageEntry(uintptr_t) { return 0; }
int processorGetCurrentId() { return 0; }
bool taskingMemoryHandleStackOverflow(g_task*, uintptr_t) { ++growth; return true; }
bool memoryOnDemandHandlePageFault(g_task*, uintptr_t) { std::abort(); }
void exceptionsDumpState(g_task*, volatile g_processor_state*) { ++dumped; }
void taskingSchedule() { ++scheduled; }
bool exceptionsTerminateUserTask(g_task*, volatile g_processor_state*) { ++scheduled; return true; }
#define G_PAGE_ALIGN_DOWN(v) ((v) & ~4095ULL)

#include "production/exceptionsHandlePageFault.inc"

TEST(PageFaults, AttemptsStackGrowthOnlyForNonPresentNonReservedFaults) {
    g_task task{}; g_processor_state state{};
    EXPECT_TRUE(exceptionsHandlePageFault(&task, &state) && growth == 1 && !dumped && !scheduled);
    state.error = 1;
    EXPECT_TRUE(exceptionsHandlePageFault(&task, &state) && growth == 1 && dumped == 1 && scheduled == 1);
    state.error = 8;
    EXPECT_TRUE(exceptionsHandlePageFault(&task, &state) && growth == 1 && dumped == 2 && scheduled == 2);
}
}
