#include "support/fixture.hpp"
#include <map>

namespace
{
using g_fd = int;
using g_offset = uint64_t;
using g_spawn_status = int;
constexpr int G_SPAWN_STATUS_SUCCESSFUL = 0, G_SPAWN_STATUS_IO_ERROR = 1, G_SPAWN_STATUS_FORMAT_ERROR = 2, G_SPAWN_STATUS_MEMORY_ERROR = 3;
#define G_ADDRESS_MAX UINTPTR_MAX
#define G_USER_MAXIMUM_HEAP_BREAK UINTPTR_MAX
#define G_PAGE_SIZE 4096ULL
#define G_PAGE_PRESENT 1
#define G_PAGE_TABLE_USER_DEFAULT 7
#define G_PAGE_USER_DEFAULT 7
#define G_PAGE_ALIGN_DOWN(v) ((v) & ~4095ULL)
#define G_PAGE_ALIGN_UP(v) (((v) + 4095ULL) & ~4095ULL)
struct Elf64_Phdr { uintptr_t p_vaddr, p_filesz, p_memsz, p_offset; };
std::map<uintptr_t, uintptr_t> entries;
int allocations, frees;
bool losePublication;
uintptr_t pagingVirtualToPageEntry(uintptr_t address) { return entries[address]; }
uintptr_t memoryPhysicalTryAllocate() { return ++allocations * 4096; }
void memoryPhysicalFree(uintptr_t) { ++frees; }
bool pagingMapPage(uintptr_t address, uintptr_t page, int, int) {
    entries[address] = page | 1; return !losePublication;
}
void memorySetBytes(void* address, int value, size_t size) { std::memset(address, value, size); }
bool filesystemReadToMemory(int, uintptr_t offset, uint8_t* address, size_t size) {
    std::memset(address, 0x41 + offset, size); return true;
}
#include "production/elfObjectValidateLoadRange.inc"
#include "production/elfObjectLoadLoadSegment.inc"

TEST(ElfSegments, ReusesSharedBoundaryPagesAndReleasesLostPublications) {
    alignas(4096) uint8_t data[8192];
    std::memset(data, 0xcc, sizeof(data));
    auto base = reinterpret_cast<uintptr_t>(data);
    Elf64_Phdr first{0, 64, 1024, 0}, adjacent{1024, 32, 128, 1};
    EXPECT_EQ(0, elfObjectLoadLoadSegment(0, first, base));
    EXPECT_TRUE(allocations == 1 && frees == 0 && data[0] == 0x41 && data[63] == 0x41);
    EXPECT_TRUE(data[64] == 0 && data[4095] == 0);
    EXPECT_EQ(0, elfObjectLoadLoadSegment(0, adjacent, base));
    EXPECT_TRUE(allocations == 1 && frees == 0 && data[0] == 0x41 && data[63] == 0x41);
    EXPECT_TRUE(data[1024] == 0x42 && data[1055] == 0x42 && data[1056] == 0);
    // A lost publication race returns its unused allocation to the allocator.
    losePublication = true;
    Elf64_Phdr racing{4096, 0, 4096, 0};
    EXPECT_EQ(0, elfObjectLoadLoadSegment(0, racing, base));
    EXPECT_TRUE(allocations == 2 && frees == 1);
}
}
