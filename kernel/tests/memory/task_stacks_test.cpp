/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2026, Max Schlüssel <lokoxe@gmail.com>                     *
 *                                                                           *
 *  This program is free software: you can redistribute it and/or modify     *
 *  it under the terms of the GNU General Public License as published by     *
 *  the Free Software Foundation, either version 3 of the License, or        *
 *  (at your option) any later version.                                      *
 *                                                                           *
 *  This program is distributed in the hope that it will be useful,          *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *  GNU General Public License for more details.                             *
 *                                                                           *
 *  You should have received a copy of the GNU General Public License        *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
 *                                                                           *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "support/fixture.hpp"

namespace
{
#include "support/memory/task_pages.hpp"

#include "production/tasking_stack.inc"

TEST(TaskStacks, KeepsGuardAndEndUnmappedAndReclaimsFailedPagePublications) {
    int pool;
    noRange = true;
    EXPECT_TRUE(!taskingMemoryCreateStack(&pool, 7, 7, 10).start && !allocations);
    reset(); failPhysical = 0;
    EXPECT_TRUE(!taskingMemoryCreateStack(&pool, 7, 7, 10).start && !ranges && !allocations);
    reset(); failMapping = 0;
    EXPECT_TRUE(!taskingMemoryCreateStack(&pool, 7, 7, 10).start && frames.empty() && !ranges);
    reset();
    auto stack = taskingMemoryCreateStack(&pool, 7, 7, 10);
    EXPECT_TRUE(stack.start == (uintptr_t)arena && stack.end == (uintptr_t)arena + 10 * 4096);
    g_task task{nullptr, stack, 2, 7};
    EXPECT_FALSE(taskingMemoryHandleStackOverflow(&task, stack.start));
    EXPECT_FALSE(taskingMemoryHandleStackOverflow(&task, stack.end));
    EXPECT_FALSE(taskingMemoryHandleStackOverflow(&task, stack.start - 1));
    EXPECT_TRUE(taskingMemoryHandleStackOverflow(&task, stack.start + 4096));
    auto before = allocations;
    EXPECT_TRUE(taskingMemoryHandleStackOverflow(&task, stack.start + 4096) && allocations == before);
    failMapping = mapAttempts;
    EXPECT_TRUE(taskingMemoryHandleStackOverflow(&task, stack.start + 8192));
    EXPECT_TRUE(allocations == before + 1 && frees == 1);
    mappings.erase(stack.start + 8192); // competing publisher's fixture frame
    while(!mappings.empty()) {
        auto it = mappings.begin(); auto p = it->second; mappings.erase(it); memoryPhysicalFree(p);
    }
    EXPECT_TRUE(frames.empty() && allocations == frees);
}

TEST(TaskStacks, CommitsAllKernelPagesAndUnwindsPartialFailures) {
    int pool;
    for(int failure : {0, 1, 3}) {
        reset(); failPhysical = failure;

        EXPECT_FALSE(taskingMemoryCreateStack(&pool, 3, 3, 9).start);
        EXPECT_TRUE(frames.empty() && allocations == frees && !ranges);

        reset(); failMapping = failure;

        EXPECT_FALSE(taskingMemoryCreateStack(&pool, 3, 3, 9).start);
        EXPECT_TRUE(frames.empty() && allocations == frees && !ranges);
    }

    reset();
    auto stack = taskingMemoryCreateStack(&pool, 3, 3, 9);

    ASSERT_NE(0U, stack.start);
    EXPECT_EQ(8U, mappings.size());
    EXPECT_FALSE(mappings.count(stack.start));
    EXPECT_FALSE(mappings.count(stack.end));
    for(auto address = stack.start + 4096; address < stack.end; address += 4096)
        EXPECT_TRUE(mappings.count(address));
    while(!mappings.empty()) {
        auto item = mappings.begin(); auto physical = item->second;
        mappings.erase(item); memoryPhysicalFree(physical);
    }

    addressRangePoolFree(&pool, stack.start);

    EXPECT_TRUE(frames.empty() && allocations == frees && !ranges);
}

TEST(TaskStacks, BatchesTeardownWithoutFreeingGuardsOrUnmanagedFrames) {
    reset();
    int pool;
    auto stack = taskingMemoryCreateStack(&pool, 3, 3, 9);
    ASSERT_NE(0U, stack.start);
    const auto unmanaged = stack.start + G_PAGE_SIZE;
    const auto physical = mappings.at(unmanaged);
    mappings.erase(unmanaged); mappingFlags.erase(unmanaged);
    memoryPhysicalFree(physical);
    mappings[unmanaged] = 0xbeef000;
    mappingFlags[unmanaged] = G_PAGE_PRESENT | G_PAGE_UNMANAGED_FLAG;
    taskingMemoryDestroyStack(&pool, stack);
    EXPECT_EQ(1, unmapBatches);
    EXPECT_TRUE(mappings.empty() && frames.empty());
    EXPECT_EQ(0, ranges); EXPECT_EQ(0U, stack.start); EXPECT_EQ(0U, stack.end);
    EXPECT_EQ(allocations, frees);
}
}
