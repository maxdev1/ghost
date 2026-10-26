#include "support/fixture.hpp"

namespace
{
#define G_PAGE_SIZE 4096ULL

#include "production/page_reference_tracker_hpp.inc"
#include "production/page_reference_tracker_cpp.inc"

TEST(PageReferences, KeepsHighPhysicalAddressesIndependent) {
    pageReferenceTrackerInitialize();
    const uintptr_t pages[] = {0x1000, 0xfffff000ULL, 0x100000000ULL, 0x200000000ULL, 0x10000000000ULL};
    for(auto page : pages) {
        pageReferenceTrackerIncrement(page);
        pageReferenceTrackerIncrement(page);
    }
    for(auto page : pages) {
        EXPECT_EQ(1, pageReferenceTrackerDecrement(page));
        EXPECT_EQ(0, pageReferenceTrackerDecrement(page));
        EXPECT_EQ(0, pageReferenceTrackerDecrement(page));
    }
    for(auto& bucket : directory.tables) {
        while(bucket) { auto next = bucket->next; delete bucket; bucket = next; }
    }
}
}
