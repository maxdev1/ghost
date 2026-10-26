#include "support/fixture.hpp"

namespace
{
#include "support/syscalls/tasking.hpp"

TEST(CreateTask, ValidatesAffinityAndSnapshotsUserEntryBeforeCreatingTask)
{
    g_task task{}; g_process process{}; task.process = &process; process.main = &task;
    task.securityLevel = G_SECURITY_LEVEL_APPLICATION;
    g_syscall_create_task create{}; create.threadId = 123;
    create.coreAffinity = 2;
    syscallCreateTask(&task, &create);
    EXPECT_TRUE(create.status == G_CREATE_TASK_STATUS_FAILED && create.threadId == G_TID_NONE && !created);
    create.coreAffinity = 1; mutateCreate = &create;
    create.userData = (void*)0x1234; create.userEntry = (void*)0x5678;
    syscallCreateTask(&task, &create);
    EXPECT_TRUE(create.status == G_CREATE_TASK_STATUS_SUCCESSFUL && create.threadId == 42 && assignedCore == 1);
    EXPECT_TRUE(child.userEntry.data == (void*)0x1234 && child.userEntry.function == (void*)0x5678);
}
}
