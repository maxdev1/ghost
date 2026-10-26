#include "support/fixture.hpp"

namespace
{
#include "support/memory/page_tables.hpp"

TEST(PageTables, ConcurrentWritersPreserveMappingsAndRejectCollisions)
{
    alignas(4096) static uint64_t root[512];
    currentSpace = reinterpret_cast<uintptr_t>(root);
    for(int round = 0; round < 40; ++round) {
        std::memset(root, 0, sizeof(root));
        nextPage = 0;
        std::atomic<int> ready{0};
        std::vector<std::thread> threads;
        for(int core = 0; core < 16; ++core) {
            threads.emplace_back([&, core] {
                ++ready;
                while(ready != 16) std::this_thread::yield();
                for(int page = core; page < 128; page += 16)
                    EXPECT_TRUE(pagingMapPage(0x40000000ULL + page * 4096,
                        0x100000000ULL + page * 4096,
                        G_PAGE_TABLE_KERNEL_DEFAULT, G_PAGE_KERNEL_DEFAULT));
            });
        }
        for(auto& thread : threads) thread.join();
        for(int page = 0; page < 128; ++page) {
            auto virt = 0x40000000ULL + page * 4096;
            EXPECT_EQ(0x100000000ULL + page * 4096, pagingVirtualToPhysical(virt));
            EXPECT_FALSE(pagingMapPage(virt, 0x200000000ULL, 3, 3));
            EXPECT_TRUE(pagingMapPage(virt, 0x200000000ULL, 3, 3, true));
            EXPECT_EQ(0x200000000ULL, pagingVirtualToPhysical(virt));
            pagingUnmapPage(virt);
            EXPECT_EQ(0, pagingVirtualToPhysical(virt));
        }
    }
}
}
