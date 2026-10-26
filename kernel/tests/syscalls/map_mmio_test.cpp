#include "support/fixture.hpp"

namespace
{
#include "production/map_mmio_callstruct.inc"
// Copy packed ABI fields before passing them to GoogleTest reference comparisons.

#define G_PAGE_SIZE 4096ULL
#define G_PAGE_ALIGN_MASK (G_PAGE_SIZE - 1)
#define G_PAGE_ALIGN_UP(p) (((p) + G_PAGE_SIZE - 1) & ~G_PAGE_ALIGN_MASK)
#define G_SECURITY_LEVEL_KERNEL 0
#define G_SECURITY_LEVEL_DRIVER 1
#define G_PROC_VIRTUAL_RANGE_FLAG_WEAK 1
#define G_PAGE_UNMANAGED_FLAG (1ULL << 9)
#define G_PAGE_TABLE_USER_DEFAULT 7
#define G_PAGE_USER_DEFAULT 7
struct g_address_range_pool {};
struct g_process { g_address_range_pool* virtualRangePool; g_mutex lock; };
struct g_task { g_process* process; int securityLevel; int id; };
constexpr uintptr_t virtualBase = 0xa0000000;
unsigned allocations, releases, mapAttempts, cpuidCalls, requestedPages;
int failAt = -1;
bool noVirtualRange;
uint32_t physicalBits = 40, maxExtendedLeaf = 0x80000008;
g_syscall_map_mmio* mutateRequest;
std::vector<std::pair<uintptr_t, uintptr_t>> mapped;
std::vector<uintptr_t> unmapped;
void processorCpuid(uint32_t leaf, uint32_t* a, uint32_t* b, uint32_t* c, uint32_t* d) {
    ++cpuidCalls;
    EXPECT_TRUE(leaf == 0x80000000 || (leaf == 0x80000008 && maxExtendedLeaf >= leaf));
    *a = leaf == 0x80000000 ? maxExtendedLeaf : physicalBits;
    *b = *c = *d = 0;
}
uintptr_t addressRangePoolAllocate(g_address_range_pool*, uint32_t pages, uint8_t flags) {
    ++allocations;
    requestedPages = pages;
    EXPECT_TRUE(flags == G_PROC_VIRTUAL_RANGE_FLAG_WEAK && pages);
    if(mutateRequest) {
        mutateRequest->physicalAddress = (1ULL << 63) | 1;
        mutateRequest->size = 0;
    }
    return noVirtualRange ? 0 : virtualBase;
}
void addressRangePoolFree(g_address_range_pool*, uintptr_t base) {
    EXPECT_TRUE(base == virtualBase && mapped.empty());
    ++releases;
}
bool pagingMapPage(uintptr_t virt, uintptr_t phys, uint64_t tableFlags, uint64_t pageFlags) {
    EXPECT_TRUE(tableFlags == G_PAGE_TABLE_USER_DEFAULT && pageFlags == (G_PAGE_USER_DEFAULT | G_PAGE_UNMANAGED_FLAG));
    if(static_cast<int>(mapAttempts++) == failAt) return false;
    mapped.emplace_back(virt, phys);
    return true;
}
void pagingUnmapPage(uintptr_t virt) {
    for(auto it = mapped.begin(); it != mapped.end(); ++it) {
        if(it->first == virt) { mapped.erase(it); unmapped.push_back(virt); return; }
    }
    ADD_FAILURE() << "rollback attempted to remove an existing mapping";
}
void memoryPhysicalFree(uintptr_t) { std::abort(); }
void pageReferenceTrackerIncrement(uintptr_t) { std::abort(); }

#include "production/physical_range.inc"
#include "production/syscallMapMmioArea.inc"

void resetFixture() {
    allocations = releases = mapAttempts = cpuidCalls = requestedPages = 0;
    failAt = -1;
    noVirtualRange = false;
    mutateRequest = nullptr;
    physicalBits = 40;
    maxExtendedLeaf = 0x80000008;
    mapped.clear();
    unmapped.clear();
}
void rejected(g_task* task, uintptr_t physical, uint32_t size) {
    g_syscall_map_mmio request{physical, size, reinterpret_cast<void*>(0x1234)};
    syscallMapMmioArea(task, &request);
    EXPECT_EQ(nullptr, static_cast<void*>(request.virtualAddress));
    EXPECT_EQ(0U, allocations);
    EXPECT_EQ(0U, mapAttempts);
    EXPECT_EQ(0U, releases);
}
void successful(g_task* task, uintptr_t physical, uint32_t size, unsigned pages) {
    g_syscall_map_mmio request{physical, size, reinterpret_cast<void*>(0x1234)};
    syscallMapMmioArea(task, &request);
    EXPECT_EQ(reinterpret_cast<void*>(virtualBase), static_cast<void*>(request.virtualAddress));
    EXPECT_EQ(1U, allocations);
    EXPECT_EQ(pages, requestedPages);
    ASSERT_EQ(pages, mapped.size());
    EXPECT_TRUE(!releases && unmapped.empty());
    for(unsigned i = 0; i < pages; ++i) {
        EXPECT_EQ(virtualBase + i * G_PAGE_SIZE, mapped[i].first);
        EXPECT_EQ(physical + i * G_PAGE_SIZE, mapped[i].second);
    }
}
class MapMmio : public testing::Test
{
protected:
    g_address_range_pool pool{};
    g_process process{&pool, {}};
    g_task driver{&process, G_SECURITY_LEVEL_DRIVER, 1};
    void SetUp() override { resetFixture(); }
};

TEST_F(MapMmio, RejectsUnprivilegedEmptyAndMisalignedRequests)
{
    for(int level : {2, 3, 255}) {
        resetFixture();
        g_task application{&process, level, 2};
        rejected(&application, 0xfd000000, 4096);
        EXPECT_FALSE(cpuidCalls);
    }
    resetFixture();
    rejected(&driver, 0xfd000000, 0);
    rejected(&driver, 0xfd000001, 4096);
    EXPECT_FALSE(cpuidCalls);
}

TEST_F(MapMmio, RoundsPageCountsForDriverAndKernelCallers)
{
    for(uint32_t size : {1U, 4096U, 4097U}) {
        resetFixture();
        successful(&driver, 0x100000000ULL, size, (size + 4095) / 4096);
    }
    resetFixture();
    g_task kernel{&process, G_SECURITY_LEVEL_KERNEL, 3};
    successful(&kernel, 0xfd000000, 4096, 1);
}

TEST_F(MapMmio, ValidatesWholeRoundedSpanAgainstCpuPhysicalWidth)
{
    // Validate the CPU limit and the entire rounded span, including its last page.
    resetFixture();
    successful(&driver, (1ULL << 40) - 4096, 4096, 1);
    resetFixture();
    rejected(&driver, (1ULL << 40) - 4096, 4097);
    rejected(&driver, 1ULL << 40, 1);
    rejected(&driver, 1ULL << 52, 1);
    rejected(&driver, 1ULL << 63, 1);
    rejected(&driver, UINTPTR_MAX - 4095, 8192);
    for(uint32_t bits : {0U, 31U, 53U, 64U}) {
        resetFixture();
        physicalBits = bits;
        rejected(&driver, 0xfd000000, 4096);
    }
    resetFixture();
    physicalBits = 52;
    successful(&driver, (1ULL << 52) - 4096, 4096, 1);
}

TEST_F(MapMmio, FallsBackTo36BitPhysicalWidthWithoutExtendedCpuid)
{
    resetFixture();
    maxExtendedLeaf = 0x80000007;
    successful(&driver, (1ULL << 36) - 4096, 4096, 1);
    EXPECT_EQ(1, cpuidCalls);
    resetFixture();
    maxExtendedLeaf = 0x80000007;
    rejected(&driver, 1ULL << 36, 1);
}

TEST_F(MapMmio, RoundsMaximumAbiSizeWithoutWrapping)
{
    // The largest ABI size rounds up to 4 GiB without wrapping to zero.
    resetFixture();
    noVirtualRange = true;
    g_syscall_map_mmio large{0xfd000000, UINT32_MAX, reinterpret_cast<void*>(0x1234)};
    syscallMapMmioArea(&driver, &large);
    EXPECT_TRUE(!large.virtualAddress && allocations == 1 && requestedPages == 1048576);
    EXPECT_TRUE(!mapAttempts && !releases);
}

TEST_F(MapMmio, SnapshotsRequestBeforeAllocatingVirtualSpace)
{
    // Changes to the user request after validation must not change the mapping.
    resetFixture();
    g_syscall_map_mmio changing{0xfd000000, 8192, nullptr};
    mutateRequest = &changing;
    syscallMapMmioArea(&driver, &changing);
    EXPECT_EQ(reinterpret_cast<void*>(virtualBase), static_cast<void*>(changing.virtualAddress));
    ASSERT_EQ(2U, mapped.size());
    EXPECT_EQ(0xfd000000U, mapped[0].second);
    EXPECT_EQ(0xfd001000, mapped[1].second);
}

TEST_F(MapMmio, RollsBackOnlyMappingsPublishedBeforeCollision)
{
    // A collision must leave that existing page alone and undo only our prefix.
    for(int failure = 0; failure < 3; ++failure) {
        resetFixture();
        failAt = failure;
        g_syscall_map_mmio request{0xfd000000, 12288, reinterpret_cast<void*>(0x1234)};
        syscallMapMmioArea(&driver, &request);
        EXPECT_TRUE(!request.virtualAddress && allocations == 1 && releases == 1);
        EXPECT_TRUE(mapAttempts == failure + 1 && mapped.empty());
        EXPECT_EQ(static_cast<unsigned>(failure), unmapped.size());
        for(int i = 0; i < failure; ++i)
            EXPECT_EQ(virtualBase + i * G_PAGE_SIZE, unmapped[i]);
    }
}

}
