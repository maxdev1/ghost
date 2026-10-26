#include "support/fixture.hpp"

namespace
{
#include "support/ipc/pipe_fixture.hpp"

TEST(PipeWaits, WaitsOnlyWhenReadOrAtomicWriteCannotProgress) {
    int id; EXPECT_EQ(0, pipeCreate(&id, true));
    published->referencesRead = published->referencesWrite = 1;
    g_task task{42, false};
    EXPECT_TRUE(pipeWaitForRead(&task, id, 16384) && task.waiting && published->waitersRead == 1);
    task.waiting = false; published->size = 16384 - 512;
    EXPECT_TRUE(pipeWaitForWrite(&task, id, 1024) && task.waiting);
    task.waiting = false;
    EXPECT_TRUE(!pipeWaitForWrite(&task, id, 16384) && !task.waiting);
    EXPECT_FALSE(pipeWaitForRead(&task, id, 16384));
    published->size = 16384;
    EXPECT_TRUE(pipeWaitForWrite(&task, id, 16384));
    published->referencesRead = 0; task.waiting = false;
    EXPECT_TRUE(!pipeWaitForWrite(&task, id, 16384) && !task.waiting);
    EXPECT_EQ(0, published->lock.depth);
    destroy();
}
}
