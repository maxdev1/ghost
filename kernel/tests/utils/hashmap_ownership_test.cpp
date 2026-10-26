#include "support/fixture.hpp"

namespace
{
#include "support/tracked_heap.hpp"
#include "production/hashmap_hpp.inc"
#include "production/hashmap_string_hpp.inc"
#include "production/hashmap_string_cpp.inc"

TEST(StringHashmap, DestroysOwnedKeysAndBucketsWithoutFreeingBorrowedValues)
{
    // Destruction owns keys and buckets, while values belong to the caller.
    auto values = hashmapCreateString<void*>(8);
    auto value = heapAllocate(32);
    hashmapPut(values, "owned-key", value);
    hashmapDestroy(values);
    EXPECT_TRUE(heapBlocks.size() == 1 && heapBlocks.count(value));
    heapFree(value);
    EXPECT_TRUE(heapBlocks.empty());
}
}
