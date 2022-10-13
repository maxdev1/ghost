#include "test/test.hpp"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// Test unit
#include "shared/memory/bitmap_page_allocator.cpp"

TEST(bitmapPageAllocator, "Simple single-bitmap test")
{
	// Means memory range of 0x80000
	uint32_t entryCount = 16;
	void* bitmapArray = malloc((sizeof(g_bitmap_header) + sizeof(g_bitmap_entry) * entryCount) * 1);

	g_bitmap_header* bitmap1 = (g_bitmap_header*) bitmapArray;
	bitmap1->baseAddress = 0x10000000;
	bitmap1->entryCount = entryCount;
	bitmap1->hasNext = false;

	g_bitmap_page_allocator allocator;
	bitmapPageAllocatorInitialize(&allocator, (g_bitmap_header*) bitmapArray);

	g_bitmap_entry* entries1 = (g_bitmap_entry*) ((g_address) bitmap1 + sizeof(g_bitmap_header));
	for(uint32_t i = 0; i < entryCount; i++)
		entries1[i] = 0b11111111;

	// Mark some pages as free and expect to get them back
	entries1[2] = 0b11110111;
	entries1[15] = 0b01111111;

	ASSERT_EQUALS((void*) 0x10013000ul, (void*) bitmapPageAllocatorAllocate(&allocator));
	ASSERT_EQUALS((void*) 0x1007f000ul, (void*) bitmapPageAllocatorAllocate(&allocator));

	for(uint32_t i = 0; i < entryCount; i++)
	{
		ASSERT_EQUALS((uint8_t) 0b11111111, entries1[i]);
	}

	// Free a valid and an invalid address
	bitmapPageAllocatorMarkFree(&allocator, 0x10051000ul);
	bitmapPageAllocatorMarkFree(&allocator, 0x20000000ul);

	for(uint32_t i = 0; i < entryCount; i++)
	{
		if(i == 10)
			ASSERT_EQUALS((uint8_t) 0b11111101, entries1[10]);
		else
			ASSERT_EQUALS((uint8_t) 0b11111111, entries1[i]);
	}

	free(bitmapArray);
}

TEST(bitmapPageAllocatorMultiple, "Multiple-bitmap test")
{
	// Means memory range of 0x80000
	uint32_t entryCount = 16;
	void* bitmapArray = malloc((sizeof(g_bitmap_header) + sizeof(g_bitmap_entry) * entryCount) * 2);

	g_bitmap_header* bitmap1 = (g_bitmap_header*) bitmapArray;
	bitmap1->baseAddress = 0x10000000;
	bitmap1->entryCount = entryCount;
	bitmap1->hasNext = true;

	g_bitmap_header* bitmap2 = G_BITMAP_NEXT(bitmap1);
	bitmap2->baseAddress = 0x4A000000;
	bitmap2->entryCount = entryCount;
	bitmap2->hasNext = false;

	g_bitmap_page_allocator allocator;
	bitmapPageAllocatorInitialize(&allocator, (g_bitmap_header*) bitmapArray);

	g_bitmap_entry* entries1 = (g_bitmap_entry*) ((g_address) bitmap1 + sizeof(g_bitmap_header));
	for(uint32_t i = 0; i < entryCount; i++)
		entries1[i] = 0b11111111;

	g_bitmap_entry* entries2 = (g_bitmap_entry*) ((g_address) bitmap2 + sizeof(g_bitmap_header));
	for(uint32_t i = 0; i < entryCount; i++)
		entries2[i] = 0b11111111;

	// Mark some pages as free and expect to get them back
	entries1[5] = 0b10111111;
	entries2[4] = 0b01111111;

	ASSERT_EQUALS((void*) 0x1002e000ul, (void*) bitmapPageAllocatorAllocate(&allocator));
	ASSERT_EQUALS((void*) 0x4a027000ul, (void*) bitmapPageAllocatorAllocate(&allocator));

	for(uint32_t i = 0; i < entryCount; i++)
		ASSERT_EQUALS((uint8_t) 0b11111111, entries1[i]);

	for(uint32_t i = 0; i < entryCount; i++)
		ASSERT_EQUALS((uint8_t) 0b11111111, entries2[i]);

	free(bitmapArray);
}

TEST(bitmapMacros, "Ensure macros calculate what we expect")
{
	{
		g_bitmap_header* first = malloc(sizeof(g_bitmap_header));
		first->entryCount = 5;

		g_bitmap_header* next = G_BITMAP_NEXT_UNCHECKED(first);
		ASSERT_EQUALS((g_bitmap_header*) (((g_address) first) + sizeof(g_bitmap_header) + 5 * sizeof(g_bitmap_entry)), next);

		free(first);
	}
	{
		g_bitmap_header* first = malloc(sizeof(g_bitmap_header));
		first->entryCount = 5;
		first->hasNext = false;

		g_bitmap_header* next = G_BITMAP_NEXT(first);
		ASSERT_EQUALS((g_bitmap_header*) nullptr, next);

		free(first);
	}
	{
		g_bitmap_header* first = malloc(sizeof(g_bitmap_header));
		first->entryCount = 5;
		first->hasNext = true;

		g_bitmap_header* next = G_BITMAP_NEXT(first);
		ASSERT_EQUALS((g_bitmap_header*) (((g_address) first) + sizeof(g_bitmap_header) + 5 * sizeof(g_bitmap_entry)), next);

		free(first);
	}
	{
		ASSERT_EQUALS(0lu, G_OFFSET_TO_BITMAP_INDEX(0x0000));
		ASSERT_EQUALS(0lu, G_OFFSET_TO_BITMAP_BIT(0x0000));
		ASSERT_EQUALS(0x0000lu, G_BITMAP_TO_OFFSET(0, 0));

		ASSERT_EQUALS(0lu, G_OFFSET_TO_BITMAP_INDEX(0x1000));
		ASSERT_EQUALS(1lu, G_OFFSET_TO_BITMAP_BIT(0x1000));
		ASSERT_EQUALS(0x1000lu, G_BITMAP_TO_OFFSET(0, 1));

		ASSERT_EQUALS(0lu, G_OFFSET_TO_BITMAP_INDEX(0x7000));
		ASSERT_EQUALS(7lu, G_OFFSET_TO_BITMAP_BIT(0x7000));
		ASSERT_EQUALS(0x7000lu, G_BITMAP_TO_OFFSET(0, 7));

		ASSERT_EQUALS(1lu, G_OFFSET_TO_BITMAP_INDEX(0x8000));
		ASSERT_EQUALS(0lu, G_OFFSET_TO_BITMAP_BIT(0x8000));
		ASSERT_EQUALS(0x8000lu, G_BITMAP_TO_OFFSET(1, 0));

		ASSERT_EQUALS(1lu, G_OFFSET_TO_BITMAP_INDEX(0xF000));
		ASSERT_EQUALS(7lu, G_OFFSET_TO_BITMAP_BIT(0xF000));
		ASSERT_EQUALS(0xF000lu, G_BITMAP_TO_OFFSET(1, 7));

		ASSERT_EQUALS(2lu, G_OFFSET_TO_BITMAP_INDEX(0x10000));
		ASSERT_EQUALS(0lu, G_OFFSET_TO_BITMAP_BIT(0x10000));
		ASSERT_EQUALS(0x10000lu, G_BITMAP_TO_OFFSET(2, 0));
	}
}
