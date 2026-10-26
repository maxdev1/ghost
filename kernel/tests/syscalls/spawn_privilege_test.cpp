#include "support/fixture.hpp"

namespace
{
#include "support/syscalls/tasking.hpp"

TEST(SpawnPrivilege, RejectsPrivilegeEscalationAndSnapshotsValidatedSecurityLevel)
{
    g_task task{}; g_process process{}; task.process = &process; process.main = &task;
    task.securityLevel = G_SECURITY_LEVEL_APPLICATION;
    g_syscall_spawn spawn{}; spawn.path = pathName;
    spawn.securityLevel = G_SECURITY_LEVEL_APPLICATION; mutateSpawn = &spawn;
    syscallSpawn(&task, &spawn);
    EXPECT_TRUE(spawnedLevel == G_SECURITY_LEVEL_APPLICATION && opened == 1);
    spawn.securityLevel = G_SECURITY_LEVEL_KERNEL;
    syscallSpawn(&task, &spawn); EXPECT_TRUE(opened == 1 && spawn.status == G_SPAWN_STATUS_TASKING_ERROR);
    spawn.securityLevel = 255;
    syscallSpawn(&task, &spawn); EXPECT_TRUE(opened == 1 && spawn.status == G_SPAWN_STATUS_TASKING_ERROR);
}
}
