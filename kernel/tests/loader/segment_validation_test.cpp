#include "support/fixture.hpp"

namespace
{
#include "support/loader/validation.hpp"

TEST(ElfSegments, RejectsOverflowingPrivilegedAndOutOfFileLoadSpans)
{
    Elf64_Phdr segment{}; segment.p_vaddr = 0x400000; segment.p_memsz = 4096; segment.p_filesz = 1024;
    EXPECT_TRUE(_elfObjectValidateLoadRange(segment, 0));
    auto reject = [](Elf64_Phdr p, uintptr_t base) {
        EXPECT_EQ(G_SPAWN_STATUS_FORMAT_ERROR, elfObjectLoadLoadSegment(0, p, base));
    };
    segment.p_filesz = 4097; reject(segment, 0);
    segment.p_filesz = 1024; segment.p_vaddr = 0xffff800000000000ULL; reject(segment, 0);
    segment.p_vaddr = 0x400000; reject(segment, UINT64_MAX - 4095);
    segment.p_vaddr = 0x9ffff000; segment.p_memsz = 8192; reject(segment, 0);
    segment.p_vaddr = 0; reject(segment, 0);
    segment.p_vaddr = 0x400000; segment.p_offset = UINT64_MAX; reject(segment, 0);
}
}
