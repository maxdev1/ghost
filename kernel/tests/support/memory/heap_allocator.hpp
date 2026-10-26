#pragma once

#include "support/synchronization.hpp"

uint64_t mathCeilToPowerOf2(uint64_t);
#include "production/allocator_hpp.inc"
#include "production/math_cpp.inc"
#include "production/allocator_cpp.inc"

class HeapAllocator : public testing::Test
{
protected:
    static constexpr size_t arenaSize = 256 * 1024;
    static constexpr size_t guardSize = 64;
    alignas(16) uint8_t storage[arenaSize + 2 * guardSize];
    uint8_t* arena = storage + guardSize;
    size_t activeSize = arenaSize;
    g_allocator allocator{};

    void initialize(size_t bytes = arenaSize)
    {
        ASSERT_LE(bytes, arenaSize);
        ASSERT_EQ(0U, bytes % G_ALLOCATOR_ALIGNMENT);
        std::memset(storage, 0xd3, sizeof(storage));
        activeSize = bytes;
        memoryAllocatorInitialize(&allocator, G_ALLOCATOR_TYPE_HEAP,
            reinterpret_cast<uintptr_t>(arena), reinterpret_cast<uintptr_t>(arena + bytes));
    }

    void SetUp() override { initialize(); }

    void expectPattern(void* memory, size_t bytes, uint8_t pattern)
    {
        auto begin = static_cast<uint8_t*>(memory);
        EXPECT_TRUE(std::all_of(begin, begin + bytes, [=](uint8_t value) { return value == pattern; }));
    }

    void expectLayout()
    {
        auto section = allocator.sections;
        uintptr_t nextAddress = reinterpret_cast<uintptr_t>(arena);
        const uintptr_t end = nextAddress + activeSize;
        bool previousFree = false;
        for(size_t count = 0; section; ++count)
        {
            ASSERT_LT(count, activeSize / sizeof(g_allocator_section_header));
            ASSERT_EQ(nextAddress, reinterpret_cast<uintptr_t>(section));
            ASSERT_LE(nextAddress + sizeof(g_allocator_section_header), end);
            ASSERT_GE(section->totalSize, sizeof(g_allocator_section_header));
            ASSERT_LE(section->totalSize, end - nextAddress);
            EXPECT_EQ(0U, nextAddress % G_ALLOCATOR_ALIGNMENT);
            EXPECT_EQ(0U, section->totalSize % G_ALLOCATOR_ALIGNMENT);
            EXPECT_LE(section->type, G_ALLOCATOR_SECTION_TYPE_BUCKET);
            const bool isFree = section->type == G_ALLOCATOR_SECTION_TYPE_FREE;
            EXPECT_FALSE(previousFree && isFree) << "adjacent free sections were not coalesced";
            previousFree = isFree;
            nextAddress += section->totalSize;
            section = section->next;
        }
        EXPECT_EQ(end, nextAddress);
    }

    void expectFullyReclaimed()
    {
        ASSERT_NE(nullptr, allocator.sections);
        EXPECT_EQ(G_ALLOCATOR_SECTION_TYPE_FREE, allocator.sections->type);
        EXPECT_EQ(activeSize, allocator.sections->totalSize);
        EXPECT_EQ(nullptr, allocator.sections->next);
    }

    void TearDown() override
    {
        EXPECT_EQ(0, allocator.lock.depth);
        expectLayout();
        expectPattern(storage, guardSize, 0xd3);
        // Includes unused backing storage, so a write beyond the active heap is caught.
        expectPattern(arena + activeSize, arenaSize - activeSize + guardSize, 0xd3);
    }
};
