#include "test/test.hpp"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// Test unit
#include "loader/memory/physical.cpp"

g_setup_information setupInformation;

TEST(memoryPhysicalReadMemoryMapSimple, "Read simple memory map")
{
	setupInformation.multibootInformation = malloc(sizeof(g_multiboot_information));

	uint32_t mmapLength = (sizeof(g_multiboot_mmap) + sizeof(uint32_t)) * 1;
	void* mmap = malloc(mmapLength);

	g_multiboot_mmap* mmap1 = (g_multiboot_mmap*) mmap;
	mmap1->size = sizeof(g_multiboot_mmap);
	mmap1->baseAddress = 0x00100000;
	mmap1->length = 0x0BF00000; // 0x00100000 - 0x0C000000
	mmap1->type = G_MULTIBOOT_MMAP_TYPE_FREE;
	setupInformation.multibootInformation->memoryMap = mmap1;
	setupInformation.multibootInformation->memoryMapLength = mmapLength;
	setupInformation.multibootInformation->flags |= G_MULTIBOOT_FLAGS_MMAP;

	uint32_t required = memoryPhysicalReadMemoryMap(0x05000000, 0);

	void* bitmapMemory = malloc(required);

	uint32_t requiredAfter = memoryPhysicalReadMemoryMap(0x05000000, bitmapMemory);
	ASSERT_EQUALS(required, requiredAfter);

	// Bitmap creates 2 splinters since the range is larger than G_BITMAP_MAX_RANGE
	g_bitmap_header* splinter1 = (g_bitmap_header*) bitmapMemory;
	ASSERT_EQUALS((g_address) 0x05000000, splinter1->baseAddress);
	ASSERT_EQUALS((uint32_t) (((0x0A000000 - 0x05000000) / G_PAGE_SIZE) / G_BITMAP_PAGES_PER_ENTRY), splinter1->entryCount);
	ASSERT_EQUALS(true, splinter1->hasNext);

	g_bitmap_header* splinter2 = G_BITMAP_NEXT(splinter1);
	ASSERT_EQUALS((g_address*) 0x0A000000, (g_address*) splinter2->baseAddress);
	ASSERT_EQUALS((uint32_t) (((0x0C000000 - 0x0A000000) / G_PAGE_SIZE) / G_BITMAP_PAGES_PER_ENTRY), splinter2->entryCount);
	ASSERT_EQUALS(false, splinter2->hasNext);

	free(bitmapMemory);
	free(mmap);
	free(setupInformation.multibootInformation);
}

TEST(memoryPhysicalReadMemoryMapTwoEntries, "Read memory map with two entries")
{
	setupInformation.multibootInformation = malloc(sizeof(g_multiboot_information));

	uint32_t mmapLength = (sizeof(g_multiboot_mmap) + sizeof(uint32_t)) * 2;
	void* mmap = malloc(mmapLength);

	g_multiboot_mmap* mmap1 = (g_multiboot_mmap*) mmap;
	mmap1->size = sizeof(g_multiboot_mmap);
	mmap1->baseAddress = 0x00100000;
	mmap1->length = 0x0BF00000; // 0x00100000 - 0x0C000000
	mmap1->type = G_MULTIBOOT_MMAP_TYPE_FREE;

	g_multiboot_mmap* mmap2 = (g_multiboot_mmap*) ((g_address) mmap + mmap1->size + sizeof(uint32_t));
	mmap2->size = sizeof(g_multiboot_mmap);
	mmap2->baseAddress = 0x2A000000;
	mmap2->length = 0x10000000; // 0x2A000000 - 0x3A000000
	mmap2->type = G_MULTIBOOT_MMAP_TYPE_FREE;

	setupInformation.multibootInformation->memoryMap = mmap1;
	setupInformation.multibootInformation->memoryMapLength = mmapLength;
	setupInformation.multibootInformation->flags |= G_MULTIBOOT_FLAGS_MMAP;

	uint32_t required = memoryPhysicalReadMemoryMap(0x05000000, 0);

	void* bitmapMemory = malloc(required);

	uint32_t requiredAfter = memoryPhysicalReadMemoryMap(0x05000000, bitmapMemory);
	ASSERT_EQUALS(required, requiredAfter);

	// Bitmap creates 6 splinters since the range is larger than G_BITMAP_MAX_RANGE
	g_bitmap_header* splinter1 = (g_bitmap_header*) bitmapMemory;
	ASSERT_EQUALS((g_address) 0x05000000, splinter1->baseAddress);
	ASSERT_EQUALS((uint32_t) (((0x0A000000 - 0x05000000) / G_PAGE_SIZE) / G_BITMAP_PAGES_PER_ENTRY), splinter1->entryCount);
	ASSERT_EQUALS(true, splinter1->hasNext);

	g_bitmap_header* splinter2 = G_BITMAP_NEXT(splinter1);
	ASSERT_EQUALS((g_address*) 0x0A000000, (g_address*) splinter2->baseAddress);
	ASSERT_EQUALS((uint32_t) (((0x0C000000 - 0x0A000000) / G_PAGE_SIZE) / G_BITMAP_PAGES_PER_ENTRY), splinter2->entryCount);
	ASSERT_EQUALS(true, splinter2->hasNext);

	g_bitmap_header* splinter3 = G_BITMAP_NEXT(splinter2);
	ASSERT_EQUALS((g_address*) 0x2A000000, (g_address*) splinter3->baseAddress);
	ASSERT_EQUALS((uint32_t) (((0x2F000000 - 0x2A000000) / G_PAGE_SIZE) / G_BITMAP_PAGES_PER_ENTRY), splinter3->entryCount);
	ASSERT_EQUALS(true, splinter3->hasNext);

	g_bitmap_header* splinter4 = G_BITMAP_NEXT(splinter3);
	ASSERT_EQUALS((g_address*) 0x2F000000, (g_address*) splinter4->baseAddress);
	ASSERT_EQUALS((uint32_t) (((0x34000000 - 0x2F000000) / G_PAGE_SIZE) / G_BITMAP_PAGES_PER_ENTRY), splinter4->entryCount);
	ASSERT_EQUALS(true, splinter4->hasNext);

	g_bitmap_header* splinter5 = G_BITMAP_NEXT(splinter4);
	ASSERT_EQUALS((g_address*) 0x34000000, (g_address*) splinter5->baseAddress);
	ASSERT_EQUALS((uint32_t) (((0x39000000 - 0x34000000) / G_PAGE_SIZE) / G_BITMAP_PAGES_PER_ENTRY), splinter5->entryCount);
	ASSERT_EQUALS(true, splinter5->hasNext);

	g_bitmap_header* splinter6 = G_BITMAP_NEXT(splinter5);
	ASSERT_EQUALS((g_address*) 0x39000000, (g_address*) splinter6->baseAddress);
	ASSERT_EQUALS((uint32_t) (((0x3A000000 - 0x39000000) / G_PAGE_SIZE) / G_BITMAP_PAGES_PER_ENTRY), splinter6->entryCount);
	ASSERT_EQUALS(false, splinter6->hasNext);

	free(bitmapMemory);
	free(mmap);
	free(setupInformation.multibootInformation);
}
