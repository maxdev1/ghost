#include "support/fixture.hpp"
#include <array>
#include <mutex>
#include <deque>

namespace
{
#include "production/spinlock_yield_hpp.inc"

#define G_MEM_LOWER_HALF_END 0x7fffffffffffULL
thread_local unsigned cpuId;
struct g_interrupt_guard {};
struct processor { unsigned id, apicId; processor* next; } topology[] = {
    {2, 47, &topology[1]}, {0, 12, &topology[2]}, {3, 83, &topology[3]}, {1, 29, nullptr}};
unsigned processorGetNumberOfProcessors() { return 4; }
processor* processorGetList() { return topology; }
unsigned processorGetCurrentIdFromApic() { return cpuId; }
std::atomic<uintptr_t> spaces[4], lastInvalidated[4]; std::atomic<unsigned> invalidations[4];
uintptr_t pagingGetCurrentSpace() { return spaces[cpuId]; }
void pagingInvalidatePage(uintptr_t address) { lastInvalidated[cpuId] = address; ++invalidations[cpuId]; }
std::atomic<unsigned> sent{0}, received{0}; std::atomic<bool> stopping{false};
std::mutex queueLock; std::deque<bool> queues[4]; std::vector<std::thread> workers;
bool pagingHandleShootdownNmi();
void lapicSendNmi(unsigned id) {
    unsigned target = 99;
    for(auto* p = topology; p; p = p->next) if(p->apicId == id) target = p->id;
    EXPECT_TRUE(target < 4 && target != cpuId);
    std::lock_guard<std::mutex> lock(queueLock); queues[target].push_back(true); ++sent;
}
void nmiWorker(unsigned id) {
    cpuId = id;
    while(!stopping) {
        bool pending;
        { std::lock_guard<std::mutex> lock(queueLock); pending = !queues[id].empty(); if(pending) queues[id].pop_front(); }
        if(pending) { EXPECT_TRUE(pagingHandleShootdownNmi()); ++received; }
        else std::this_thread::yield();
    }
}

#include "production/paging_shootdown_host_cpp.inc"

TEST(TlbShootdown, FiltersAddressSpacesAndAcknowledgesConcurrentMailboxes) {
    EXPECT_FALSE(pagingHandleShootdownNmi());
    for(unsigned i = 0; i < 4; ++i) { spaces[i] = 0x1000; workers.emplace_back(nmiWorker, i); }
    spaces[2] = 0x2000;
    pagingSynchronizePage(0x4000); EXPECT_TRUE(invalidations[0] == 1 && sent == 0);
    pagingInitializeSmp();
    pagingSynchronizePage(0x5000);
    EXPECT_TRUE(invalidations[0] == 2 && invalidations[1] == 1 && invalidations[2] == 0 && invalidations[3] == 1);
    EXPECT_EQ(0x5000U, lastInvalidated[0].load());
    EXPECT_EQ(0x5000U, lastInvalidated[1].load());
    EXPECT_EQ(0U, lastInvalidated[2].load());
    EXPECT_EQ(0x5000U, lastInvalidated[3].load());
    // Global mappings must be invalidated even on a different CR3.
    pagingSynchronizePage(0xffffff8000100000ULL);
    EXPECT_EQ(1, invalidations[2]);
    for(auto& address : lastInvalidated) EXPECT_EQ(UINT64_C(0xffffff8000100000), address.load());
    std::thread a([] { cpuId = 0; for(int i = 0; i < 200; ++i) pagingSynchronizePage(0x9000); });
    std::thread b([] { cpuId = 1; for(int i = 0; i < 200; ++i) pagingSynchronizePage(0xa000); });
    a.join(); b.join();
    while(received != sent) std::this_thread::yield();
    EXPECT_TRUE(sent == 3 * 402 && received == sent);
    stopping = true; for(auto& worker : workers) worker.join();
}
}
