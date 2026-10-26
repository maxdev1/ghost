#include "support/fixture.hpp"

namespace
{
#include "support/tracked_heap.hpp"

#define G_SSE_STATE_SIZE 512
#define G_SSE_STATE_ALIGNMENT 16
#define G_ALIGN_UP(v, a) (((v) + (a) - 1) & ~((uintptr_t)(a) - 1))
enum class g_cpuid_standard_edx_feature { SSE };
struct g_task;
struct g_process { g_task* main; uintptr_t pageSpace; };
struct g_task {
    g_process* process;
    struct { uint8_t* stateMem; uint8_t* state; bool stored; } fpu;
    uintptr_t overridePageDirectory;
    struct { void* userThreadLocal; void* kernelThreadLocal; } threadLocal;
    struct { uintptr_t end; } interruptStack;
};
uint8_t initial[512], hardware[512]; int restored;
bool processorHasFeature(g_cpuid_standard_edx_feature) { return true; }
void* processorGetInitialFpuState() { return initial; }
void memoryCopy(void* a, const void* b, size_t n) { std::memcpy(a, b, n); }
void pagingSwitchToSpace(uintptr_t) {}
void gdtSetTlsAddresses(void*, void*) {}
void gdtSetTssRsp0(uintptr_t) {}
void processorRestoreFpuState(void* p) { std::memcpy(hardware, p, 512); ++restored; }

#include "production/tasking_fpu.inc"
#include "production/taskingRestoreState.inc"

TEST(FpuState, RestoresInitialAndInheritedImagesOnFirstDispatch) {
    for(size_t i = 0; i < sizeof(initial); ++i) initial[i] = static_cast<uint8_t>(i * 37);
    g_process process{}; process.pageSpace = 0x1000;
    g_task main{}; main.process = &process; process.main = &main;
    taskingMemoryInitializeUtility(&main);
    ASSERT_TRUE(main.fpu.stored);
    ASSERT_NE(nullptr, main.fpu.state);
    EXPECT_EQ(0U, reinterpret_cast<uintptr_t>(main.fpu.state) % 16);
    taskingRestoreState(&main);
    EXPECT_EQ(1, restored);
    EXPECT_EQ(0, std::memcmp(hardware, initial, sizeof(hardware)));
    main.fpu.state[0] = 0x55;
    g_task sibling{}; sibling.process = &process;
    taskingMemoryInitializeUtility(&sibling);
    ASSERT_TRUE(sibling.fpu.stored);
    ASSERT_NE(nullptr, sibling.fpu.state);
    taskingRestoreState(&sibling);
    EXPECT_EQ(2, restored);
    EXPECT_EQ(0, std::memcmp(hardware, main.fpu.state, sizeof(hardware)));
    ASSERT_NE(main.fpu.state, sibling.fpu.state);
    sibling.fpu.state[0] = 0x99;
    EXPECT_EQ(0x55, main.fpu.state[0]);
    taskingMemoryDestroyUtility(&main); taskingMemoryDestroyUtility(&sibling);
    EXPECT_TRUE(heapBlocks.empty());
    EXPECT_FALSE(main.fpu.stored);
    EXPECT_EQ(nullptr, main.fpu.state);
    EXPECT_FALSE(sibling.fpu.stored);
    EXPECT_EQ(nullptr, sibling.fpu.state);
}
}
