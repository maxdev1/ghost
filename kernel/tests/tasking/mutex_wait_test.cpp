#define mutexAcquire fixtureMutexAcquire
#include "support/fixture.hpp"
#undef mutexAcquire

static constexpr int G_MUTEX_INITIALIZED = 0xfeed;
static constexpr unsigned G_MUTEX_MAX_PAUSES = 1024;
struct g_task { unsigned id = 17; } mutexTask;
struct g_tasking_local { struct { int globalLockCount = 0; } locking; } mutexLocal;
static bool mutexInterrupts, mutexExpectedInterrupts, mutexContended;
static unsigned mutexAttempts, mutexYields;
bool interruptsAreEnabled() { return mutexInterrupts; }
void interruptsDisable() { mutexInterrupts = false; }
void interruptsEnable() { mutexInterrupts = true; }
bool systemIsReady() { return true; }
unsigned processorGetCurrentId() { return 3; }
g_task* taskingGetCurrentTask() { return &mutexTask; }
g_tasking_local* taskingGetLocal() { return &mutexLocal; }
void mutexErrorUninitialized(g_mutex*) { FAIL() << "Unexpected uninitialized mutex"; }
bool _mutexTryAcquire(g_mutex*, uint32_t owner, bool hadIF)
{
    EXPECT_FALSE(mutexInterrupts);
    EXPECT_EQ(mutexTask.id, owner);
    EXPECT_EQ(mutexExpectedInterrupts, hadIF);
    return ++mutexAttempts > 1 || !mutexContended;
}
void taskingYield()
{
    ++mutexYields;
    EXPECT_EQ(mutexExpectedInterrupts, mutexInterrupts);
}
#include "production/mutex_acquire.inc"

TEST(TaskMutexWait, PreservesCallerInterruptsWhileYieldingAndOnAcquisition)
{
    for(bool enabled : {false, true})
    {
        for(bool contended : {false, true})
        {
            mutexInterrupts = mutexExpectedInterrupts = enabled;
            mutexContended = contended;
            mutexAttempts = mutexYields = 0;
            g_mutex mutex{};
            mutex.initialized = G_MUTEX_INITIALIZED;
            mutex.type = G_MUTEX_TYPE_TASK;
            mutexAcquire(&mutex);
            EXPECT_EQ(enabled, mutexInterrupts);
            EXPECT_EQ(contended ? 1U : 0U, mutexYields);
        }
    }
}
