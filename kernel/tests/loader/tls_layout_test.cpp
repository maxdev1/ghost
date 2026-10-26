#include "support/fixture.hpp"

namespace
{
#include "support/tracked_heap.hpp"

using g_fd = int; using g_spawn_status = int;
constexpr int G_SPAWN_STATUS_SUCCESSFUL = 0, G_SPAWN_STATUS_FORMAT_ERROR = 1;
constexpr int G_SPAWN_STATUS_MEMORY_ERROR = 2, G_SPAWN_STATUS_IO_ERROR = 3;
constexpr uintptr_t G_USER_VIRTUAL_RANGES_START = 0, G_USER_VIRTUAL_RANGES_END = 65536;
#define G_ADDRESS_MAX UINTPTR_MAX
#define G_ALIGN_UP(v,a) (((v) + (a) - 1) & ~((a) - 1))
struct Elf64_Phdr { uint64_t p_offset, p_filesz, p_memsz, p_align; };
struct g_user_threadlocal { uintptr_t self; int tid; };
struct g_elf_object {
    bool root; g_elf_object* parent;
    struct { uint8_t* content; uint64_t alignment, copysize, totalsize, offset; } tlsPart;
    struct { uint64_t totalSize, userThreadOffset; } tlsMaster;
};
bool failRead, failHeap;
void* heapTryAllocate(uint32_t size) { return failHeap ? nullptr : heapAllocate(size ? size : 1); }
bool filesystemReadToMemory(int, uint64_t, uint8_t* out, uint64_t length) {
    for(uint64_t i = 0; i < length; ++i) out[i] = i;
    return !failRead;
}

#include "production/elfTlsLoadData.inc"

TEST(ElfTls, ValidatesLayoutAndHandlesLibrariesWithoutExecutableTls) {
    g_elf_object root{}, library{}; root.root = true; library.parent = &root;
    Elf64_Phdr phdr{0, 7, 21, 16};
    EXPECT_EQ(0, elfTlsLoadData(0, phdr, &library));
    EXPECT_TRUE(library.tlsPart.offset >= sizeof(g_user_threadlocal));
    EXPECT_TRUE(library.tlsPart.offset % 16 == 0 && root.tlsMaster.userThreadOffset == 0);
    EXPECT_EQ(library.tlsPart.offset + 32, root.tlsMaster.totalSize);
    for(int i = 0; i < 7; ++i) EXPECT_EQ(i, library.tlsPart.content[i]);
    EXPECT_EQ(G_SPAWN_STATUS_FORMAT_ERROR, elfTlsLoadData(0, phdr, &library));
    heapFree(library.tlsPart.content);
    for(auto alignment : {0ULL, 1ULL, 8ULL, 32ULL, 4096ULL}) {
        root = {}; root.root = true; phdr = {0, 0, 17, alignment};
        EXPECT_EQ(0, elfTlsLoadData(0, phdr, &root));
        EXPECT_TRUE(root.tlsMaster.userThreadOffset >= 17);
        EXPECT_EQ(0, root.tlsMaster.userThreadOffset % alignof(g_user_threadlocal));
        EXPECT_EQ(root.tlsMaster.userThreadOffset + sizeof(g_user_threadlocal), root.tlsMaster.totalSize);
        heapFree(root.tlsPart.content);
    }
    for(auto bad : {Elf64_Phdr{0, 8, 7, 8}, Elf64_Phdr{0, 1, 7, 3},
                    Elf64_Phdr{UINT64_MAX, 2, 7, 8}, Elf64_Phdr{0, 0, 65537, 8}}) {
        root = {}; root.root = true;
        EXPECT_EQ(G_SPAWN_STATUS_FORMAT_ERROR, elfTlsLoadData(0, bad, &root));
        EXPECT_TRUE(heapBlocks.empty());
    }
    root = {}; root.root = true; phdr = {0, 7, 21, 16};
    failRead = true; EXPECT_EQ(G_SPAWN_STATUS_IO_ERROR, elfTlsLoadData(0, phdr, &root));
    failRead = false; failHeap = true;
    EXPECT_EQ(G_SPAWN_STATUS_MEMORY_ERROR, elfTlsLoadData(0, phdr, &root));
    EXPECT_TRUE(heapBlocks.empty());
}
}
