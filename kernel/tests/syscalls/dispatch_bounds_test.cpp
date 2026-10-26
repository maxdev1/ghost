#include "support/fixture.hpp"

namespace
{
constexpr unsigned G_SYSCALL_MAX = 1;
struct g_task { int id, state; } task{};
g_task* taskingGetCurrentTask() { return &task; }
void taskingHandleTerminationRequest() {}
void interruptsEnable() {} void interruptsDisable() {}
int calls; void handler(g_task*, void*) { ++calls; }
struct g_syscall_registration { void (*handler)(g_task*, void*); bool interruptible; };
g_syscall_registration registrations[1], *syscallRegistrations = registrations;

#include "production/syscall.inc"

TEST(SyscallDispatch, RejectsNumbersAtAndAboveTheExclusiveLimit) {
    registrations[0] = {handler, false};
    syscall(G_SYSCALL_MAX, nullptr); syscall(UINT32_MAX, nullptr);
    EXPECT_FALSE(calls); syscall(0, nullptr); EXPECT_EQ(1, calls);
}
}
