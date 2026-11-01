#include <gtest/gtest.h>
#include <cstdint>
#include <sys/types.h>
#define GHOST_API_STDINT
using g_bool = uint8_t;
#include <ghost/filesystem.h>

extern "C" {
extern __thread int ghost_errno;
int ghost_remove(const char*);
int ghost_unlink(const char*);
int ghost_mkdir(const char*, mode_t);
int ghost_rmdir(const char*);
int ghost_ftruncate(int, off_t);
int ghost_fsync(int);
}

static g_fs_mutation_status mutationStatus, directoryStatus;
static unsigned directoryCalls;
static uint64_t requestedLength;
g_fs_mutation_status g_create_directory(const char*) { return mutationStatus; }
g_fs_mutation_status g_unlink(const char*) { return mutationStatus; }
g_fs_mutation_status g_unlink_directory(const char*) { ++directoryCalls; return directoryStatus; }
g_fs_mutation_status g_truncate(g_fd, uint64_t length) { requestedLength = length; return mutationStatus; }
g_fs_mutation_status g_sync(g_fd) { return mutationStatus; }

TEST(FilesystemMutation, ReportsSuccessAndTranslatesFilesystemErrors)
{
	mutationStatus = directoryStatus = G_FS_MUTATION_SUCCESSFUL;
	EXPECT_EQ(0, ghost_mkdir("folder", 0777));
	EXPECT_EQ(0, ghost_unlink("file"));
	EXPECT_EQ(0, ghost_rmdir("folder"));
	EXPECT_EQ(0, ghost_ftruncate(42, 1234));
	EXPECT_EQ(1234U, requestedLength);
	EXPECT_EQ(0, ghost_fsync(42));
	EXPECT_EQ(-1, ghost_fsync(-1));
	EXPECT_EQ(8, ghost_errno);
	EXPECT_EQ(-1, ghost_ftruncate(-1, 1234));
	EXPECT_EQ(8, ghost_errno);
	mutationStatus = G_FS_MUTATION_NO_SPACE;
	EXPECT_EQ(-1, ghost_mkdir("folder", 0777));
	EXPECT_EQ(53, ghost_errno);
	mutationStatus = G_FS_MUTATION_NOT_FOUND;
	EXPECT_EQ(-1, ghost_fsync(42));
	EXPECT_EQ(8, ghost_errno);
}

TEST(FilesystemMutation, RemoveTriesDirectoriesOnlyForWrongNodeType)
{
	directoryCalls = 0;
	mutationStatus = G_FS_MUTATION_NOT_FOUND;
	EXPECT_EQ(-1, ghost_remove("missing"));
	EXPECT_EQ(0U, directoryCalls);
	mutationStatus = G_FS_MUTATION_INVALID_ARGUMENT;
	directoryStatus = G_FS_MUTATION_NOT_EMPTY;
	EXPECT_EQ(-1, ghost_remove("folder"));
	EXPECT_EQ(1U, directoryCalls);
	EXPECT_EQ(59, ghost_errno);
	requestedLength = 99;
	EXPECT_EQ(-1, ghost_ftruncate(42, -1));
	EXPECT_EQ(99U, requestedLength);
}
