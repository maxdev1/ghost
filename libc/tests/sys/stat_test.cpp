#include <gtest/gtest.h>
#include <cstdint>
#define GHOST_API_STDINT
using g_bool = uint8_t;
#include <ghost/filesystem.h>
extern "C" {
extern __thread int ghost_errno;
int ghost_stat(const char*, void*);
void _stat_from_g_fs_stat(void*, g_fs_stat_data*) {}
}
static g_fs_stat_status statStatus;
g_fs_stat_status g_fs_stat(const char*, g_fs_stat_data*) { return statStatus; }
TEST(Stat, ReportsMissingPathsAndIoErrorsThroughErrno)
{
	alignas(16) uint8_t buffer[512];
	statStatus = G_FS_STAT_NOT_FOUND; EXPECT_EQ(-1, ghost_stat("missing", buffer)); EXPECT_EQ(46, ghost_errno);
	statStatus = G_FS_STAT_ERROR; EXPECT_EQ(-1, ghost_stat("failed", buffer)); EXPECT_EQ(30, ghost_errno);
	statStatus = G_FS_STAT_SUCCESS; EXPECT_EQ(0, ghost_stat("present", buffer));
}
