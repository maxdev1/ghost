#include "support/fixture.hpp"
#include <functional>

namespace
{
#include "support/tracked_heap.hpp"

using g_pid = int;
using g_tid = int;
using g_user_mutex = uint32_t;
constexpr int G_TID_NONE = -1;
struct g_wait_queue { int unused; };
void waitQueueInitialize(g_wait_queue*) {}
void waitQueueAdd(g_wait_queue*, int) {}
void waitQueueRemove(g_wait_queue*, int) {}
void waitQueueWake(g_wait_queue*) {}
struct g_process { int id; } a{1}, b{2};
struct g_task { int id; g_process* process; } owner{7, &a}, other{8, &b};
g_task* current = &owner;
g_task* taskingGetCurrentTask() { return current; }
void taskingPrepareWait(g_task*, const char*) {}
struct clock_state { uint64_t time; } clockState{0};
clock_state* clockGetLocal() { return &clockState; }
void clockWaitForTime(int, uint64_t) {}
void clockUnwaitForTime(int) {}
bool clockHasTimedOut(int) { return false; }
std::function<void()> onYield;
int yields;
template<class F> bool taskingWaitIf(const F& prepare) {
    bool wait = prepare();
    if(wait) { ++yields; EXPECT_TRUE(onYield); onYield(); }
    return wait;
}
#include "production/user_mutex_layout.inc"

void userMutexUnwaitForAcquire(g_user_mutex, g_tid);

#include "production/hashmap_hpp.inc"
#include "production/user_mutex_cpp.inc"

TEST(UserMutexes, ReleasesDeadOwnersAndReresolvesDestroyedHandles) {
    userMutexInitialize();
    current = &owner;
    auto owned = userMutexCreate(true);
    EXPECT_EQ(G_USER_MUTEX_STATUS_ACQUIRED, userMutexTryAcquire(&owner, owned));
    EXPECT_EQ(G_USER_MUTEX_STATUS_ACQUIRED, userMutexTryAcquire(&owner, owned));
    current = &other;
    auto foreign = userMutexCreate(false);
    EXPECT_EQ(G_USER_MUTEX_STATUS_NOT_ACQUIRED, userMutexTryAcquire(&other, owned));
    userMutexTaskRemoved(owner.id);
    EXPECT_EQ(G_USER_MUTEX_STATUS_ACQUIRED, userMutexTryAcquire(&other, owned));
    userMutexProcessRemoved(owner.process->id);
    EXPECT_EQ(G_USER_MUTEX_STATUS_NOT_ACQUIRED, userMutexTryAcquire(&other, owned));
    EXPECT_EQ(1, hashmapSize(mutexMap));
    EXPECT_EQ(G_USER_MUTEX_STATUS_ACQUIRED, userMutexTryAcquire(&other, foreign));
    // Destroy a contended entry while an acquire is asleep. The next iteration
    // must re-resolve its handle instead of using a freed entry pointer.
    current = &owner;
    onYield = [=] { userMutexDestroy(foreign); };
    EXPECT_EQ(G_USER_MUTEX_STATUS_NOT_ACQUIRED, userMutexAcquire(&owner, foreign, 0, false));
    EXPECT_TRUE(yields == 1 && hashmapSize(mutexMap) == 0);
    userMutexProcessRemoved(other.process->id);
    hashmapDestroy(mutexMap);
    EXPECT_TRUE(heapBlocks.empty());
}
}
