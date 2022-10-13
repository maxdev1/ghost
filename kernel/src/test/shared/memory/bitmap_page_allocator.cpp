#include "test/test.hpp"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// Test unit
#include "shared/memory/bitmap_page_allocator.cpp"

TEST(bitmapPageAllocator, "Simple single-bitmap test")
{
	uint32_t entryCount = 16; // means memory range of 0x80000
	void* bitmapArray = malloc((sizeof(g_bitmap_header) + sizeof(g_bitmap_entry) * entryCount));

	g_bitmap_header* bitmap = (g_bitmap_header*) bitmapArray;
	bitmap->baseAddress = 0x10000000;
	bitmap->entryCount = entryCount;
	bitmap->hasNext = false;

	g_bitmap_page_allocator allocator;
	bitmapPageAllocatorInitialize(&allocator, bitmap);

	g_bitmap_entry* entries = (g_bitmap_entry*) (bitmapArray + sizeof(g_bitmap_header));
	for(uint32_t i = 0; i < entryCount; i++)
		entries[i] = 0b11111111;

	// Get back expected pages
	entries[2] = 0b11110111;
	entries[15] = 0b01111111;

	ASSERT_EQUALS((void*) 0x10013000ul, (void*) bitmapPageAllocatorAllocate(&allocator));
	ASSERT_EQUALS((void*) 0x1007f000ul, (void*) bitmapPageAllocatorAllocate(&allocator));

	for(uint32_t i = 0; i < entryCount; i++)
		ASSERT_EQUALS((uint8_t) 0b11111111, entries[i]);

	// Free a page
	bitmapPageAllocatorMarkFree(&allocator, 0x10051000ul);
	for(uint32_t i = 0; i < entryCount; i++)
	{
		if(i == 10)
			ASSERT_EQUALS((uint8_t) 0b11111101, entries[10]);
		else
			ASSERT_EQUALS((uint8_t) 0b11111111, entries[i]);
	}

	free(bitmapArray);
}
