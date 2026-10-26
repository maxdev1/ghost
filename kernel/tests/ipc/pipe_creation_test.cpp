#include "support/fixture.hpp"

namespace
{
#include "support/ipc/pipe_fixture.hpp"

TEST(PipeCreation, DistinguishesStreamAndPacketCapacityAndCleansUpAllocationFailure) {
    int id = -1;
    EXPECT_EQ(G_FS_PIPE_SUCCESSFUL, pipeCreate(&id, false));
    EXPECT_TRUE(published->capacity == 1024 && allocatedPages == 1);
    destroy();
    EXPECT_EQ(G_FS_PIPE_SUCCESSFUL, pipeCreate(&id, true));
    EXPECT_TRUE(published->capacity == G_PIPE_STREAM_CAPACITY && allocatedPages == 4);
    destroy();
    failAllocation = true; id = -1;
    EXPECT_TRUE(pipeCreate(&id, true) == G_FS_PIPE_ERROR && id == -1 && heapBlocks.empty());
}
}
