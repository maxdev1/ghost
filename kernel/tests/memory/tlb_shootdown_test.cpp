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
#include <array>
#include <mutex>
#include <deque>

namespace
{
#include "production/spinlock_yield_hpp.inc"

#include "production/paging_shootdown_hpp.inc"
#include "production/timer_configuration.inc"
#define G_MEM_LOWER_HALF_END 0x7fffffffffffULL
#define G_PAGE_SIZE 4096ULL
thread_local unsigned cpuId;
struct g_interrupt_guard {};
struct processor { unsigned id, apicId; processor* next; } topology[] = {
    {2, 47, &topology[1]}, {0, 12, &topology[2]}, {3, 83, &topology[3]}, {1, 29, nullptr}};
unsigned processorGetNumberOfProcessors() { return 4; }
processor* processorGetList() { return topology; }
unsigned processorGetCurrentIdFromApic() { return cpuId; }
unsigned processorGetCurrentId() { return cpuId; }
std::atomic<uintptr_t> spaces[4], lastInvalidated[4]; std::atomic<unsigned> invalidations[4];
std::atomic<unsigned> spaceFlushes[4];
bool pagingHandleShootdownIpi();
void pagingPollShootdown();
std::atomic<unsigned> pollTarget{99}, injectIpiTarget{99}, nestedIpis{0};
uintptr_t pagingGetCurrentSpace() { return spaces[cpuId]; }
void pagingSwitchToSpace(uintptr_t space) { EXPECT_EQ(spaces[cpuId].load(), space); ++spaceFlushes[cpuId]; }
void pagingInvalidatePage(uintptr_t address) {
    lastInvalidated[cpuId] = address;
    ++invalidations[cpuId];
    unsigned expected = cpuId;
    if(injectIpiTarget.compare_exchange_strong(expected, 99)) {
        EXPECT_TRUE(pagingHandleShootdownIpi());
        ++nestedIpis;
    }
}
std::atomic<unsigned> sent{0}, received{0}; std::atomic<bool> stopping{false};
std::atomic<unsigned> broadcasts{0};
std::atomic<unsigned> dropTarget{99}, duplicateTarget{99}, dropped{0};
std::atomic<unsigned> holdTarget{99};
std::atomic<bool> holdReached{false}, originReturned{false};
std::mutex queueLock; std::deque<bool> queues[4]; std::vector<std::thread> workers;
void lapicSendIpi(unsigned id, uint8_t vector) {
    EXPECT_EQ(G_TLB_SHOOTDOWN_VECTOR, vector);
    unsigned target = 99;
    for(auto* p = topology; p; p = p->next) if(p->apicId == id) target = p->id;
    EXPECT_TRUE(target < 4 && target != cpuId);
    unsigned expected = target;
    if(dropTarget.compare_exchange_strong(expected, 99)) { ++dropped; return; }
    std::lock_guard<std::mutex> lock(queueLock); queues[target].push_back(true); ++sent;
    expected = target;
    if(duplicateTarget.compare_exchange_strong(expected, 99)) { queues[target].push_back(true); ++sent; }
}
void lapicSendIpiAllOthers(uint8_t vector);
void ipiWorker(unsigned id) {
    cpuId = id;
    while(!stopping) {
        bool pending;
        { std::lock_guard<std::mutex> lock(queueLock); pending = !queues[id].empty(); if(pending) queues[id].pop_front(); }
        if(pending) {
            if(holdTarget == id) {
                holdReached = true;
                while(holdTarget == id) std::this_thread::yield();
            }
            EXPECT_TRUE(pagingHandleShootdownIpi()); ++received;
        }
        else {
            if(pollTarget == id) pagingPollShootdown();
            std::this_thread::yield();
        }
    }
}

#include "production/paging_shootdown_host_cpp.inc"

void lapicSendIpiAllOthers(uint8_t vector) {
    ++broadcasts;
    for(auto* p = topology; p; p = p->next) if(p->id != cpuId) {
        EXPECT_EQ(sequence, memoryAtomicLoad(&mailboxes[p->id].requested, G_MEMORY_ORDER_ACQUIRE));
        lapicSendIpi(p->apicId, vector);
    }
}

void delayedShootdownOrigin() {
    cpuId = 0;
    pagingSynchronizePages(0xe000, 256);
    originReturned = true;
}

TEST(TlbShootdown, FiltersAddressSpacesAndAcknowledgesConcurrentMailboxes) {
    EXPECT_FALSE(pagingHandleShootdownIpi());
    for(unsigned i = 0; i < 4; ++i) { spaces[i] = 0x1000; workers.emplace_back(ipiWorker, i); }
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
    EXPECT_EQ(402u, broadcasts.load());

    auto previousSent = sent.load();
    auto previousInvalidations = invalidations[1].load();
    dropTarget = 1;
    pagingSynchronizePage(0xb000);
    while(received != sent) std::this_thread::yield();
    EXPECT_EQ(1U, dropped.load());
    EXPECT_EQ(previousSent + 3, sent.load());
    EXPECT_EQ(previousInvalidations + 1, invalidations[1].load());
    EXPECT_EQ(0xb000U, lastInvalidated[1].load());

    previousSent = sent.load();
    previousInvalidations = invalidations[1].load();
    pollTarget = injectIpiTarget = dropTarget = 1;
    pagingSynchronizePage(0xd000);
    while(received != sent) std::this_thread::yield();
    EXPECT_EQ(2U, dropped.load());
    EXPECT_EQ(1U, nestedIpis.load());
    EXPECT_EQ(previousSent + 2, sent.load());
    EXPECT_EQ(previousInvalidations + 1, invalidations[1].load());
    EXPECT_EQ(0xd000U, lastInvalidated[1].load());
    pollTarget = 99;

    previousSent = sent.load();
    previousInvalidations = invalidations[1].load();
    duplicateTarget = 1;
    pagingSynchronizePage(0xc000);
    while(received != sent) std::this_thread::yield();
    EXPECT_EQ(previousSent + 4, sent.load());
    EXPECT_EQ(previousInvalidations + 1, invalidations[1].load());
    EXPECT_EQ(0xc000U, lastInvalidated[1].load());

    // Publishing all mailboxes and sending a broadcast cannot permit reuse
    // while even one recipient has not yet performed its invalidation.
    holdTarget = 1;
    std::thread delayed(delayedShootdownOrigin);
    while(!holdReached) std::this_thread::yield();
    EXPECT_FALSE(originReturned);
    holdTarget = 99;
    delayed.join();
    EXPECT_TRUE(originReturned);
    EXPECT_EQ(1u, spaceFlushes[0].load());
    EXPECT_EQ(1u, spaceFlushes[1].load());
    EXPECT_EQ(0u, spaceFlushes[2].load());
    EXPECT_EQ(1u, spaceFlushes[3].load());
    previousInvalidations = invalidations[2].load();
    previousSent = broadcasts.load();
    pagingSynchronizePages(0xffffff8000100000ULL, 3);
    while(received != sent) std::this_thread::yield();
    EXPECT_EQ(previousSent + 1, broadcasts.load());
    EXPECT_EQ(previousInvalidations + 3, invalidations[2].load());
    EXPECT_EQ(UINT64_C(0xffffff8000102000), lastInvalidated[2].load());
    previousInvalidations = invalidations[2].load();
    pagingSynchronizePages(0x10000, 2, true);
    while(received != sent) std::this_thread::yield();
    EXPECT_EQ(previousInvalidations + 2, invalidations[2].load());
    EXPECT_EQ(0x11000u, lastInvalidated[2].load());

    stopping = true; for(auto& worker : workers) worker.join();
}
}
