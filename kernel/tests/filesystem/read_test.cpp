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
#include <condition_variable>

namespace
{
#include "support/filesystem/descriptors.hpp"

class FilesystemRead : public ::testing::Test
{
protected:
    g_file_descriptor* descriptor = nullptr;

    void SetUp() override
    {
        filesystemProcessInitialize();
        filesystemProcessCreate(42);
        ASSERT_EQ(G_FS_OPEN_SUCCESSFUL,
                  filesystemProcessCreateDescriptor(42, 1, 0, &descriptor, 7, 2));
        mode = 0;
        readStatus = G_FS_READ_SUCCESSFUL;
        readCount = 1;
        readCalls = 0;
        delegate.waitForRead = waitNode;
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

TEST_F(FilesystemRead, AdvancesOffsetByPartialReadCount)
{
    int64_t count = -1;

    EXPECT_EQ(G_FS_READ_SUCCESSFUL, filesystemRead(&task, 7, nullptr, 8, &count));
    EXPECT_EQ(1, count);
    EXPECT_EQ(2, readOffset);
    EXPECT_EQ(3, descriptor->offset);
}

TEST_F(FilesystemRead, BlockingEndOfFileReturnsWithoutWaiting)
{
    descriptor->openFlags = G_FILE_FLAG_MODE_BLOCKING;
    readCount = 0;
    int64_t count = -1;

    EXPECT_EQ(G_FS_READ_SUCCESSFUL, filesystemRead(&task, 7, nullptr, 8, &count));
    EXPECT_EQ(0, count);
    EXPECT_EQ(1, readCalls);
    EXPECT_EQ(2, descriptor->offset);
    EXPECT_FALSE(task.waiting);
}

TEST_F(FilesystemRead, ReadErrorLeavesOffsetAndReleasesLock)
{
    readStatus = G_FS_READ_ERROR;
    readCount = -1;
    int64_t count = 99;

    EXPECT_EQ(G_FS_READ_ERROR, filesystemRead(&task, 7, nullptr, 8, &count));
    EXPECT_EQ(-1, count);
    EXPECT_EQ(2, descriptor->offset);
}

TEST_F(FilesystemRead, NonblockingBusyReturnsWithoutWaiting)
{
    readStatus = G_FS_READ_BUSY;
    readCount = 0;
    int64_t count = -1;

    EXPECT_EQ(G_FS_READ_BUSY, filesystemRead(&task, 7, nullptr, 8, &count));
    EXPECT_EQ(0, count);
    EXPECT_EQ(1, readCalls);
    EXPECT_FALSE(task.waiting);
    EXPECT_EQ(2, descriptor->offset);
}

TEST_F(FilesystemRead, MissingWaitImplementationReturnsError)
{
    descriptor->openFlags = G_FILE_FLAG_MODE_BLOCKING;
    delegate.waitForRead = nullptr;
    readStatus = G_FS_READ_BUSY;
    readCount = 0;
    int64_t count = -1;

    EXPECT_EQ(G_FS_READ_ERROR, filesystemRead(&task, 7, nullptr, 8, &count));
    EXPECT_EQ(1, readCalls);
    EXPECT_FALSE(task.waiting);
}

TEST_F(FilesystemRead, RetriesAtCurrentOffsetWhenReadyBeforeSleep)
{
    descriptor->openFlags = G_FILE_FLAG_MODE_BLOCKING;
    readStatus = G_FS_READ_BUSY;
    readCount = 0;
    delegate.waitForRead = [](g_task*, g_fs_node*, uint64_t)
    {
        auto info = hashmapGet(filesystemProcessInfo, 42, (g_filesystem_process*)nullptr);
        auto open = hashmapGet(info->descriptors, 7, (g_file_descriptor*)nullptr);

        EXPECT_EQ(0, open->operationLock.depth);

        open->offset = 7;
        readStatus = G_FS_READ_SUCCESSFUL;
        readCount = 1;
        return false;
    };

    int64_t count = -1;

    EXPECT_EQ(G_FS_READ_SUCCESSFUL, filesystemRead(&task, 7, nullptr, 8, &count));
    EXPECT_EQ(2, readCalls);
    EXPECT_EQ(1, count);
    EXPECT_EQ(7, readOffset);
    EXPECT_EQ(8, descriptor->offset);
    EXPECT_FALSE(task.waiting);
}
}
