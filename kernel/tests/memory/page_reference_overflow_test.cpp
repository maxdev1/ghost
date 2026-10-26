#include "support/fixture.hpp"

namespace
{
#define G_PAGE_SIZE 4096ULL

#include "production/page_reference_tracker_hpp.inc"
#include "production/page_reference_tracker_cpp.inc"

TEST(PageReferences, CountsMoreThan32768SharedReferencesWithoutWrapping) {
    pageReferenceTrackerInitialize();
    for(int i = 0; i < 40000; ++i) pageReferenceTrackerIncrement(0x100000000ULL);
    for(int i = 39999; i >= 0; --i)
        EXPECT_EQ(static_cast<uint32_t>(i), pageReferenceTrackerDecrement(0x100000000ULL));
    for(auto& bucket : directory.tables)
        while(bucket) { auto next = bucket->next; delete bucket; bucket = next; }
}
}
