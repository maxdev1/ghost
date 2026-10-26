#include "support/fixture.hpp"

namespace
{
using g_fd = int32_t;
using g_bool = uint8_t;
#include "production/task_api_types.inc"
#include "production/wait_queue_hpp.inc"
struct g_process;
struct g_tasking_local;
struct g_processor_state;
struct g_task_information_vm86;
#include "production/task_layout.inc"

TEST(TaskLayout, KeepsProductionTaskMutexAndAtomicFieldsNaturallyAligned)
{
    static_assert(alignof(g_mutex) >= alignof(int), "mutex atomics require natural alignment");
    static_assert(offsetof(g_task, lock) % alignof(g_mutex) == 0, "task locks require natural alignment");
    g_task task{};
    EXPECT_EQ(0U, reinterpret_cast<uintptr_t>(&task.lock) % alignof(g_mutex));
    EXPECT_EQ(0U, reinterpret_cast<uintptr_t>(&task.lock.initialized) % alignof(int));
    EXPECT_EQ(0U, reinterpret_cast<uintptr_t>(&task.lock.lock) % alignof(g_spinlock));
    EXPECT_EQ(0U, reinterpret_cast<uintptr_t>(&task.references) % alignof(uint32_t));
}
}
