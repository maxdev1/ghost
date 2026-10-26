#include "support/fixture.hpp"
#include <condition_variable>

namespace
{
#include "support/filesystem/descriptors.hpp"

class FilesystemWrite : public ::testing::Test
{
protected:
    g_file_descriptor* descriptor = nullptr;

    void SetUp() override
    {
        filesystemProcessInitialize();
        filesystemProcessCreate(42);
        ASSERT_EQ(G_FS_OPEN_SUCCESSFUL,
                  filesystemProcessCreateDescriptor(42, 1, 0, &descriptor, 7, 2));
        writeStatus = G_FS_WRITE_SUCCESSFUL;
        writeCount = 1;
        writeCalls = 0;
        lengthStatus = G_FS_LENGTH_SUCCESSFUL;
        delegate.waitForWrite = waitNode;
        task.waiting = false;
        cancel = false;
    }

    void TearDown() override
    {
        cancel = false;
        EXPECT_EQ(0, descriptor->operationLock.depth);
        EXPECT_EQ(1, descriptor->references);
        filesystemProcessReleaseDescriptor(descriptor);
        filesystemProcessRemove(42);
        hashmapDestroy(filesystemProcessInfo);
        EXPECT_TRUE(heapBlocks.empty());
    }
};

TEST_F(FilesystemWrite, AdvancesOffsetByPartialWriteCount)
{
    int64_t count = -1;
    EXPECT_EQ(G_FS_WRITE_SUCCESSFUL, filesystemWrite(&task, 7, nullptr, 8, &count));
    EXPECT_EQ(1, count);
    EXPECT_EQ(2, writeOffset);
    EXPECT_EQ(3, descriptor->offset);
}

TEST_F(FilesystemWrite, AppendsAtFileLength)
{
    descriptor->openFlags = G_FILE_FLAG_MODE_APPEND;
    int64_t count = -1;
    EXPECT_EQ(G_FS_WRITE_SUCCESSFUL, filesystemWrite(&task, 7, nullptr, 8, &count));
    EXPECT_EQ(1, count);
    EXPECT_EQ(4, writeOffset);
    EXPECT_EQ(5, descriptor->offset);
}

TEST_F(FilesystemWrite, FailedAppendLengthLeavesOffsetAndReleasesLock)
{
    descriptor->openFlags = G_FILE_FLAG_MODE_APPEND;
    lengthStatus = 1;
    int64_t count = 99;
    EXPECT_EQ(G_FS_WRITE_ERROR, filesystemWrite(&task, 7, nullptr, 8, &count));
    EXPECT_EQ(-1, count);
    EXPECT_EQ(0, writeCalls);
    EXPECT_EQ(2, descriptor->offset);
}

TEST_F(FilesystemWrite, NonblockingBusyReturnsWithoutWaiting)
{
    writeStatus = G_FS_WRITE_BUSY;
    writeCount = 0;
    int64_t count = -1;
    EXPECT_EQ(G_FS_WRITE_BUSY, filesystemWrite(&task, 7, nullptr, 8, &count));
    EXPECT_EQ(0, count);
    EXPECT_EQ(1, writeCalls);
    EXPECT_FALSE(task.waiting);
    EXPECT_EQ(2, descriptor->offset);
}

TEST_F(FilesystemWrite, MissingWaitImplementationReturnsError)
{
    descriptor->openFlags = G_FILE_FLAG_MODE_BLOCKING;
    delegate.waitForWrite = nullptr;
    writeStatus = G_FS_WRITE_BUSY;
    writeCount = 0;
    int64_t count = -1;
    EXPECT_EQ(G_FS_WRITE_ERROR, filesystemWrite(&task, 7, nullptr, 8, &count));
    EXPECT_EQ(1, writeCalls);
    EXPECT_FALSE(task.waiting);
}

TEST_F(FilesystemWrite, RetriesWhenReadyBeforeSleepAndRechecksAppendPosition)
{
    descriptor->openFlags = G_FILE_FLAG_MODE_BLOCKING | G_FILE_FLAG_MODE_APPEND;
    writeStatus = G_FS_WRITE_BUSY;
    writeCount = 0;
    delegate.waitForWrite = [](g_task*, g_fs_node*, uint64_t)
    {
        EXPECT_EQ(0, descriptorLock.depth);
        EXPECT_EQ(4, writeOffset);
        // Another operation moved the offset while this write was busy.
        auto info = hashmapGet(filesystemProcessInfo, 42, (g_filesystem_process*)nullptr);
        auto open = hashmapGet(info->descriptors, 7, (g_file_descriptor*)nullptr);
        EXPECT_EQ(0, open->operationLock.depth);
        open->offset = 30;
        writeStatus = G_FS_WRITE_SUCCESSFUL;
        writeCount = 1;
        return false;
    };
    int64_t count = -1;
    EXPECT_EQ(G_FS_WRITE_SUCCESSFUL, filesystemWrite(&task, 7, nullptr, 8, &count));
    EXPECT_EQ(2, writeCalls);
    EXPECT_EQ(1, count);
    EXPECT_EQ(4, writeOffset);
    EXPECT_EQ(5, descriptor->offset);
    EXPECT_FALSE(task.waiting);
}

TEST_F(FilesystemWrite, CancellationReleasesReferenceAndOperationLock)
{
    descriptor->openFlags = G_FILE_FLAG_MODE_BLOCKING;
    writeStatus = G_FS_WRITE_BUSY;
    writeCount = 0;
    delegate.waitForWrite = [](g_task*, g_fs_node*, uint64_t)
    {
        cancel = true;
        return false;
    };
    int64_t count = -1;
    EXPECT_THROW(filesystemWrite(&task, 7, nullptr, 8, &count), Cancelled);
}
}
