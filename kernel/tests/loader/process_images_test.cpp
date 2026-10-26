#include "support/fixture.hpp"

namespace
{
#define G_PAGE_SIZE 4096ULL
#define G_PAGE_ALIGN_UP(v) (((v) + 4095ULL) & ~4095ULL)
#define G_MEM_PHYS_TO_VIRT(p) (p)
#define G_PROC_VIRTUAL_RANGE_FLAG_INTERNAL 2
#define G_USER_MAXIMUM_HEAP_BREAK UINTPTR_MAX
#define G_PAGE_TABLE_USER_DEFAULT 7
#define G_PAGE_USER_DEFAULT 7
using g_fd = int; using g_security_level = int;
struct g_elf_object;
struct object_entry { g_elf_object* value; };
struct object_list { std::vector<object_entry> entries; };
struct object_iterator { object_list* list; size_t index; };
struct g_elf_object {
    const char* name;
    struct { uint32_t totalSize, userThreadOffset; } tlsMaster;
    struct { uint8_t* content; uint32_t offset, copysize; } tlsPart;
    object_list* loadedObjects;
    void *preinitArray, *init, *initArray, *fini, *finiArray;
    uint32_t preinitArraySize, initArraySize, finiArraySize;
};
struct g_object_info {
    char* name;
    void *preinitArray, *init, *initArray, *fini, *finiArray;
    uint32_t preinitArraySize, initArraySize, finiArraySize;
};
struct g_user_threadlocal { void* self; };
void memoryPhysicalFree(uintptr_t) { std::abort(); }
struct g_process_info { int objectInfosSize; g_object_info* objectInfos; void (*syscallKernelEntry)(uint32_t, void*); };
struct g_process {
    int* virtualRangePool;
    struct { uintptr_t location; uint32_t size, userThreadOffset; } tlsMaster;
    g_process_info* userProcessInfo;
};
alignas(4096) uint8_t arena[8192]; int allocated, mapped;
uintptr_t addressRangePoolAllocate(int*, uint32_t pages, uint8_t flags) {
    EXPECT_TRUE(pages == 1 && flags == G_PROC_VIRTUAL_RANGE_FLAG_INTERNAL); return (uintptr_t)arena;
}
uintptr_t memoryPhysicalTryAllocate() { return (uintptr_t)arena + allocated++ * 4096; }
void memorySetBytes(void* p, int v, size_t n) { std::memset(p, v, n); }
void memoryCopy(void* a, const void* b, size_t n) { std::memcpy(a, b, n); }
bool pagingMapPage(uintptr_t v, uintptr_t p, int, int) {
    EXPECT_EQ(p, v); for(size_t i = 0; i < 4096; ++i) EXPECT_EQ(0, ((uint8_t*)p)[i]); ++mapped; return true;
}
object_iterator hashmapIteratorStart(object_list* list) { return {list, 0}; }
bool hashmapIteratorHasNext(object_iterator* it) { return it->index < it->list->entries.size(); }
object_entry* hashmapIteratorNext(object_iterator* it) { return &it->list->entries[it->index++]; }
void hashmapIteratorEnd(object_iterator*) {}
size_t stringLength(const char* p) { return std::strlen(p); }
void stringCopy(char* a, const char* b) { std::strcpy(a, b); }
void syscall(uint32_t, void*) {}

#include "production/elfTlsCreateMasterImage.inc"
#include "production/elfUserProcessCreateInfo.inc"

TEST(ElfProcessImages, ZeroesTlsAndProcessInfoPagePadding) {
    std::memset(arena, 0xa5, sizeof(arena));
    uint8_t first[4] = {1, 2, 3, 4}, second[4] = {5, 6, 7, 8};
    g_elf_object root{}, library{};
    root.tlsMaster = {48, 16}; root.tlsPart = {first, 0, 4}; root.name = "root";
    library.tlsPart = {second, 32, 4}; library.name = "lib";
    object_list objects{{{&root}, {&library}}}; root.loadedObjects = &objects;
    g_process process{};
    ASSERT_TRUE(elfTlsCreateMasterImage(0, &process, &root));
    EXPECT_TRUE(process.tlsMaster.size == 4096 && process.tlsMaster.location == (uintptr_t)arena);
    EXPECT_TRUE(std::memcmp(arena, first, 4) == 0 && std::memcmp(arena + 32, second, 4) == 0);
    for(size_t i = 48; i < 4096; ++i) EXPECT_EQ(0, arena[i]);
    auto start = (uintptr_t)arena + 4096;
    ASSERT_EQ(start + 4096, elfUserProcessCreateInfo(&process, &root, start, 2));
    ASSERT_NE(nullptr, process.userProcessInfo);
    EXPECT_TRUE(process.userProcessInfo->objectInfosSize == 2 && mapped == 2);
    const size_t used = sizeof(g_process_info) + 2 * sizeof(g_object_info) + 5 + 4;
    for(size_t i = used; i < 4096; ++i) EXPECT_EQ(0, arena[4096 + i]);
}
}
