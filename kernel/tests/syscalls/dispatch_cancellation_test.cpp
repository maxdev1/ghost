#include "support/fixture.hpp"

namespace
{
constexpr unsigned G_SYSCALL_MAX = 1;
struct g_task { int id, state; } task{7, 17};
g_task* taskingGetCurrentTask() { return &task; }
bool irq;
int checks, calls;
void interruptsEnable() { irq = true; }
void interruptsDisable() { irq = false; }
void taskingHandleTerminationRequest() {
    EXPECT_TRUE(!irq && task.state == 17);
    if(checks++ == 0) EXPECT_EQ(0, calls); else EXPECT_EQ(1, calls);
}
void handler(g_task* t, void*) { EXPECT_TRUE(irq && checks == 1); t->state = 42; ++calls; }
struct g_syscall_registration { void (*handler)(g_task*, void*); bool interruptible; };
g_syscall_registration registrations[2], *syscallRegistrations = registrations;

#include "production/syscall.inc"

TEST(SyscallDispatch, ChecksTerminationWithInterruptsDisabledBeforeAndAfterHandler) {
    registrations[0] = {handler, true};
    syscall(0, nullptr);
    EXPECT_TRUE(checks == 2 && calls == 1 && !irq && task.state == 17);
}
}
