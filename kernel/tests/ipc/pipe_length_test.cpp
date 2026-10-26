#include "support/fixture.hpp"

namespace
{
using g_fs_phys_id = int;
using g_fs_length_status = int;
constexpr int G_FS_LENGTH_SUCCESSFUL = 1, G_FS_LENGTH_ERROR = 2;
struct g_pipeline { g_mutex lock; uint64_t size; } pipe{{}, 1ULL << 40};
g_pipeline* pipeGetById(int id) { return id == 1 ? &pipe : nullptr; }

#include "production/pipeGetLength.inc"

TEST(Pipes, Preserves64BitLengthAndReportsMissingHandles) {
    uint64_t size = 123;
    EXPECT_TRUE(pipeGetLength(2, &size) == G_FS_LENGTH_ERROR && size == 123);
    EXPECT_TRUE(pipeGetLength(1, &size) == G_FS_LENGTH_SUCCESSFUL && size == pipe.size && pipe.lock.depth == 0);
}
}
