#include "test/test.hpp"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// Test unit
#include "kernel/memory/chunk_allocator.cpp"

TEST(chunkAllocatorInitialize, "Initialize allocator")
{
	uint8_t* testMemory = malloc(0x1000);

	g_chunk_allocator alloc;
	g_virtual_address start = (g_virtual_address) testMemory;
	g_virtual_address end = (g_virtual_address) testMemory + 0x1000;
	chunkAllocatorInitialize(&alloc, start, end);

	ASSERT_EQUALS((g_chunk_header*) start, alloc.first);
	ASSERT_EQUALS((uint8_t) false, alloc.first->used);
	ASSERT_EQUALS((uint32_t) (0x1000 - sizeof(g_chunk_header)), alloc.first->size);
	ASSERT_EQUALS((g_chunk_header*) nullptr, alloc.first->next);

	free(testMemory);
}

TEST(chunkAllocatorExpand, "Expand memory")
{
	uint8_t* testMemory = malloc(0x1000);

	g_chunk_allocator alloc;
	g_virtual_address start = (g_virtual_address) testMemory;
	g_virtual_address end = (g_virtual_address) testMemory + 0x500;
	chunkAllocatorInitialize(&alloc, start, end);

	chunkAllocatorExpand(&alloc, 0x100);
	ASSERT_EQUALS((uint32_t) (0x600 - sizeof(g_chunk_header)), alloc.first->size);
	ASSERT_EQUALS((g_chunk_header*) nullptr, alloc.first->next);

	free(testMemory);
}

TEST(chunkAllocatorAllocate, "Allocation")
{
	uint8_t* testMemory = malloc(0x1000);

	g_chunk_allocator alloc;
	g_virtual_address start = (g_virtual_address) testMemory;
	g_virtual_address end = (g_virtual_address) testMemory + 0x1000;
	chunkAllocatorInitialize(&alloc, start, end);

	void* chunk1 = chunkAllocatorAllocate(&alloc, 0x70);
	ASSERT_EQUALS((void*) (start + sizeof(g_chunk_header)), chunk1);

	auto chunk2Header = alloc.first->next;
	ASSERT_NOT_EQUALS((g_chunk_header*) nullptr, chunk2Header);
	ASSERT_EQUALS((g_chunk_header*) (start + sizeof(g_chunk_header) + 0x70), chunk2Header);
	ASSERT_EQUALS((uint32_t) (0x1000 - sizeof(g_chunk_header) - 0x70 - sizeof(g_chunk_header)), chunk2Header->size);

	auto expectedChunk2 = ((g_address) chunk2Header) + sizeof(g_chunk_header);
	auto chunk2 = chunkAllocatorAllocate(&alloc, 0x100);
	ASSERT_EQUALS(chunk2, (void*) expectedChunk2);

	free(testMemory);
}

TEST(chunkAllocatorFreeInbetween, "Free something in between")
{
	uint8_t* testMemory = malloc(0x1000);

	g_chunk_allocator alloc;
	g_virtual_address start = (g_virtual_address) testMemory;
	g_virtual_address end = (g_virtual_address) testMemory + 0x1000;
	chunkAllocatorInitialize(&alloc, start, end);

	auto chunk1 = chunkAllocatorAllocate(&alloc, 0x70);
	ASSERT_EQUALS((void*) (start + sizeof(g_chunk_header)), chunk1);
	auto chunk2 = chunkAllocatorAllocate(&alloc, 0x100);
	ASSERT_EQUALS((void*) (start + sizeof(g_chunk_header) + 0x70 + sizeof(g_chunk_header)), chunk2);
	auto chunk3 = chunkAllocatorAllocate(&alloc, 0x120);
	ASSERT_EQUALS((void*) (start + sizeof(g_chunk_header) + 0x70 + sizeof(g_chunk_header) + 0x100 + sizeof(g_chunk_header)), chunk3);

	// Free chunk 2
	chunkAllocatorFree(&alloc, chunk2);

	g_chunk_header* header1 = alloc.first;
	ASSERT_EQUALS((uint32_t) 0x70, header1->size);
	ASSERT_EQUALS((uint8_t) true, header1->used);

	g_chunk_header* header2 = header1->next;
	ASSERT_EQUALS((uint32_t) 0x100, header2->size);
	ASSERT_EQUALS((g_chunk_header*) (start + sizeof(g_chunk_header) + header1->size), header2);
	ASSERT_EQUALS((uint8_t) false, header2->used);

	g_chunk_header* header3 = header2->next;
	ASSERT_EQUALS((uint32_t) 0x120, header3->size);
	ASSERT_EQUALS((g_chunk_header*) (start + sizeof(g_chunk_header) + header1->size + sizeof(g_chunk_header) + header2->size), header3);
	ASSERT_EQUALS((uint8_t) true, header3->used);

	g_chunk_header* header4 = header3->next;
	ASSERT_EQUALS((g_chunk_header*) (start + sizeof(g_chunk_header) + header1->size + sizeof(g_chunk_header) + header2->size + sizeof(g_chunk_header) + header3->size), header4);
	ASSERT_EQUALS((uint8_t) false, header4->used);

	// Free chunk 1
	chunkAllocatorFree(&alloc, chunk1);

	header1 = alloc.first;
	uint32_t newSize = 0x100 + sizeof(g_chunk_header) + 0x70;
	ASSERT_EQUALS((uint32_t) newSize, header1->size);
	ASSERT_EQUALS((uint8_t) false, header1->used);

	header2 = header1->next;
	ASSERT_EQUALS((g_chunk_header*) (start + sizeof(g_chunk_header) + newSize), header2);
	ASSERT_EQUALS((uint8_t) true, header2->used);

	header3 = header2->next;
	ASSERT_EQUALS((g_chunk_header*) (start + sizeof(g_chunk_header) + newSize + sizeof(g_chunk_header) + header2->size), header3);
	ASSERT_EQUALS((uint8_t) false, header3->used);

	ASSERT_EQUALS((g_chunk_header*) nullptr, header3->next);

	free(testMemory);
}
