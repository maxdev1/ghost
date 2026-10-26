#include "support/fixture.hpp"

namespace
{
#include "support/memory/page_tables.hpp"

TEST(PageTables, RejectsTableExhaustionWithoutPublishingParent)
{
    for(unsigned existingLevels = 0; existingLevels < 3; ++existingLevels)
    {
        SCOPED_TRACE(existingLevels);
        alignas(4096) uint64_t root[512] = {}, pdpt[512] = {}, pd[512] = {};
        const auto rootEntry = existingLevels >= 1 ? reinterpret_cast<uintptr_t>(pdpt) | 7 : 0;
        const auto pdptEntry = existingLevels >= 2 ? reinterpret_cast<uintptr_t>(pd) | 7 : 0;
        root[0] = rootEntry;
        pdpt[0] = pdptEntry;
        currentSpace = reinterpret_cast<uintptr_t>(root);
        noMemory = true;
        EXPECT_FALSE(pagingMapPage(0x1000, 0x9000, 7, 7));
        EXPECT_EQ(rootEntry, root[0]);
        EXPECT_EQ(pdptEntry, pdpt[0]);
        EXPECT_EQ(0U, pd[0]);
        EXPECT_EQ(0U, nextPage.load());
    }
}
}
