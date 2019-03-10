#include "test/test.hpp"
#include <stdint.h>

#define __GHOST_SYS_TYPES__
#define __GHOST_SYS_TYPES__
typedef uint64_t g_virtual_address;
typedef uint64_t g_address;
typedef uint64_t g_physical_address;



#include "kernel/memory/chunk_allocator.cpp"


uint8_t testMemory[0x1000];
TEST(chunkAllocatorInitialize)
{
	g_chunk_allocator alloc;
	g_virtual_address start = (g_virtual_address) testMemory;
	g_virtual_address end = (g_virtual_address) testMemory + 0x1000;

	chunkAllocatorInitialize(&alloc, start, end);

	ASSERT_EQUALS(start, alloc.first);
	ASSERT_EQUALS(false, alloc.first->used);
	ASSERT_EQUALS(0x1000 - sizeof(g_chunk_header), alloc.first->size);
	ASSERT_EQUALS(0, alloc.first->next);
}


TEST(chunkAllocatorExpand)
{
	g_chunk_allocator alloc;
	g_virtual_address start = (g_virtual_address) testMemory;
	g_virtual_address end = (g_virtual_address) testMemory + 0x500;
	chunkAllocatorInitialize(&alloc, start, end);

	ASSERT_EQUALS(start, alloc.first);
	ASSERT_EQUALS(false, alloc.first->used);
	ASSERT_EQUALS(0x500 - sizeof(g_chunk_header), alloc.first->size);
	ASSERT_EQUALS(0, alloc.first->next);

	chunkAllocatorExpand(&alloc, 0x100);
	g_chunk_header* next = alloc.first->next;
	ASSERT_EQUALS(next, alloc.first->next);

	ASSERT_EQUALS(start, next);
	ASSERT_EQUALS(false, next->used);
	ASSERT_EQUALS(0x100 - sizeof(g_chunk_header), next->size);
	ASSERT_EQUALS(0, next->next);
}

