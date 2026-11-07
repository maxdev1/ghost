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

TEST(SpawnLaunchMode, InheritsOverridesAndPublishesBeforeChildRelease)
{
    g_task task{}; g_process process{}; task.process = &process; process.main = &task;
    task.securityLevel = G_SECURITY_LEVEL_APPLICATION;
    process.environment.launchMode = G_LAUNCH_MODE_UI;
    mutateSpawn = nullptr; spawnSuccess = true;
    g_syscall_spawn request{}; request.path = pathName; request.securityLevel = G_SECURITY_LEVEL_APPLICATION;
    request.mode = G_LAUNCH_MODE_INHERIT;
    syscallSpawn(&task, &request);

    EXPECT_EQ(G_SPAWN_STATUS_SUCCESSFUL, request.status);
    EXPECT_EQ(G_LAUNCH_MODE_UI, releasedMode);

    request.mode = G_LAUNCH_MODE_TERMINAL;
    syscallSpawn(&task, &request);

    EXPECT_EQ(G_LAUNCH_MODE_TERMINAL, releasedMode);

    request.mode = G_LAUNCH_MODE_INHERIT;
    syscallSpawn(&task, &request);

    EXPECT_EQ(G_LAUNCH_MODE_UI, releasedMode);

    int before = opened;
    request.mode = 42;
    syscallSpawn(&task, &request);

    EXPECT_EQ(before, opened);
    EXPECT_EQ(G_SPAWN_STATUS_TASKING_ERROR, request.status);

    g_syscall_get_launch_mode mode{};
    syscallGetLaunchMode(&task, &mode);

    EXPECT_EQ(G_LAUNCH_MODE_UI, mode.mode);

    process.environment.launchMode = G_LAUNCH_MODE_UNKNOWN;
    syscallGetLaunchMode(&task, &mode);

    EXPECT_EQ(G_LAUNCH_MODE_UNKNOWN, mode.mode);
}
}
