#include "support/fixture.hpp"

namespace
{
constexpr uintptr_t G_MEM_LOWER_HALF_END = 0x7fffffffffffULL;
constexpr uintptr_t G_PAGE_SIZE = 4096, G_PAGE_ALIGN_MASK = 4095;
constexpr uintptr_t G_PAGE_PRESENT = 1, G_PAGE_USER_FLAG = 4;
constexpr uintptr_t G_PAGE_UNMANAGED_FLAG = 512, G_PAGE_LARGE_PAGE_FLAG = 128;
constexpr uintptr_t G_PAGE_PHYSICAL_MASK = 0x000ffffffffff000ULL;
alignas(4096) uint8_t arena[3 * 4096]; uintptr_t entries[4]; int calls;
#define G_MEM_PHYS_TO_VIRT(p) (reinterpret_cast<uintptr_t>(arena) + (p))
uintptr_t pagingVirtualToPageEntry(uintptr_t addr) { return addr / 4096 < 4 ? entries[addr / 4096] : 0; }
void memoryCopy(void* out, const void* in, size_t count) { std::memcpy(out, in, count); }
struct g_process { g_mutex lock; };
struct g_task { g_process* process; struct { uintptr_t start, end; } stack; };
struct g_processor_state { uintptr_t cs, rip, rbp; };
void exceptionsPrintCallAtRip(g_task*, uintptr_t) { ++calls; }

#include "production/exception_stack_trace.inc"

TEST(FaultStackTrace, StopsAtCyclesInvalidUserMappingsAndKernelFrames) {
    g_process process{}; g_task task{&process, {0x1000, 0x4000}};
    g_processor_state state{3, 0x6000, 0x2000};
    const uintptr_t valid = G_PAGE_PRESENT | G_PAGE_USER_FLAG;
    entries[2] = 0x1000 | valid; entries[3] = 0x2000 | valid;
    auto words = reinterpret_cast<uintptr_t*>(arena + 0x1000);
    words[0] = 0x2010; words[1] = 0x7000;
    words[2] = 0x2010; words[3] = 0x8000; // cyclic chain
    exceptionsPrintStackTrace(&task, &state); EXPECT_TRUE(calls == 3 && process.lock.depth == 0);
    for(auto rbp : {0ULL, 0x1000ULL, 0x2001ULL, 0x3ff8ULL, 0xffffffffffffffffULL}) {
        state.rbp = rbp; calls = 0; exceptionsPrintStackTrace(&task, &state); EXPECT_EQ(1, calls);
    }
    for(auto flags : {G_PAGE_PRESENT, valid | G_PAGE_UNMANAGED_FLAG, valid | G_PAGE_LARGE_PAGE_FLAG}) {
        entries[2] = 0x1000 | flags; state.rbp = 0x2000;
        calls = 0; exceptionsPrintStackTrace(&task, &state); EXPECT_EQ(1, calls);
    }
    entries[2] = 0x1000 | valid; entries[3] = 0;
    uint8_t copy[32]; EXPECT_FALSE(exceptionsReadUser(0x2ff8, copy, 32)); // crosses an absent page
    EXPECT_FALSE(exceptionsReadUser(UINTPTR_MAX, copy, 1));
    state.cs = 0; calls = 0; exceptionsPrintStackTrace(&task, &state); EXPECT_EQ(0, calls);
}
}
