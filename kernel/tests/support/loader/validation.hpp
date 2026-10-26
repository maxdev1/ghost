#pragma once
#include "production/elf64_types.inc"
#include "production/spawn_validation.inc"

using g_spawn_validation_details = int; using g_spawn_status = int; using g_fd = int; using g_offset = uint64_t;
constexpr int G_SPAWN_STATUS_SUCCESSFUL = 0, G_SPAWN_STATUS_IO_ERROR = 1, G_SPAWN_STATUS_FORMAT_ERROR = 2, G_SPAWN_STATUS_MEMORY_ERROR = 3;
#define G_ADDRESS_MAX UINT64_MAX
#define G_USER_MAXIMUM_HEAP_BREAK 0xa0000000ULL
#define G_PAGE_SIZE 4096ULL
#define G_PAGE_PRESENT 1
#define G_PAGE_TABLE_USER_DEFAULT 7
#define G_PAGE_USER_DEFAULT 7
#define G_PAGE_ALIGN_DOWN(v) ((v) & ~4095ULL)
#define G_PAGE_ALIGN_UP(v) (((v) + 4095ULL) & ~4095ULL)
uintptr_t pagingVirtualToPageEntry(uintptr_t) { std::abort(); }
uintptr_t memoryPhysicalTryAllocate() { std::abort(); }
void memoryPhysicalFree(uintptr_t) { std::abort(); }
bool pagingMapPage(uintptr_t, uintptr_t, int, int) { std::abort(); }
void memorySetBytes(void*, int, size_t) { std::abort(); }
bool filesystemReadToMemory(int, uintptr_t, uint8_t*, size_t) { std::abort(); }

#include "production/elfValidateHeader.inc"
#include "production/elf_load_segment.inc"

