#include "support/fixture.hpp"

namespace
{
#include "support/synchronization.hpp"
struct g_address_range_pool; void addressRangePoolMerge(g_address_range_pool*) {}

#include "production/address_range_pool_hpp.inc"
#include "production/address_range_free.inc"

TEST(AddressRanges, GrantsOneReleaseClaimAndKeepsRangeReservedUntilFree) {
    g_address_range range{nullptr, true, 0x1000, 8, 1};
    g_address_range_pool pool{&range, {}}; std::atomic<int> winners{0};
    std::vector<std::thread> threads;
    for(int i = 0; i < 8; ++i) threads.emplace_back([&] {
        uint32_t pages = 0;
        if(addressRangePoolClaimForFree(&pool, 0x1000, &pages)) { ++winners; EXPECT_EQ(8, pages); }
    });
    for(auto& t : threads) t.join();
    EXPECT_TRUE(winners == 1 && range.used && (range.flags & G_ADDRESS_RANGE_FLAG_RELEASING));
    EXPECT_TRUE(addressRangePoolFree(&pool, 0x1000) == 8 && !range.used && !range.flags);
    uint32_t pages = 99; EXPECT_TRUE(!addressRangePoolClaimForFree(&pool, 0x1000, &pages) && pages == 99);
}
}
