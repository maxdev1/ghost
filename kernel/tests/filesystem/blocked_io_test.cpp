#include "support/fixture.hpp"
#include <condition_variable>

namespace
{
#include "support/filesystem/descriptors.hpp"


void make(int fd, int node = 1) {
    g_file_descriptor* created;
    EXPECT_EQ(0, filesystemProcessCreateDescriptor(42, node, 1, &created, fd));
    filesystemProcessReleaseDescriptor(created);
}
void waitPrepared() {
    std::unique_lock<std::mutex> lock(phaseLock);
    phaseChanged.wait(lock, [] { return waitingPrepared; });
}
void resume() {
    std::lock_guard<std::mutex> lock(phaseLock); proceed = true; phaseChanged.notify_all();
}
TEST(DescriptorLifetime, CancelsBlockedIoWithoutFollowingReusedDescriptorHandles) {
    filesystemProcessInitialize();
    for(int round = 0; round < 50; ++round) {
        filesystemProcessCreate(42); make(7); mode = 1;
        waitingPrepared = proceed = cancel = false;
        int status = -1; int64_t count = 99;
        std::thread reader([&] { status = filesystemRead(&task, 7, nullptr, 1, &count); });
        waitPrepared();
        // No task-owned pin remains while the I/O is sleeping.
        auto info = hashmapGet(filesystemProcessInfo, 42, (g_filesystem_process*)nullptr);
        auto original = hashmapGet(info->descriptors, 7, (g_file_descriptor*)nullptr);
        EXPECT_TRUE(original->references == 0 && task.waiting);
        const auto before = closedNodes.load();
        EXPECT_TRUE(filesystemProcessCloseDescriptor(42, 7) == 0 && closedNodes == before + 1);
        EXPECT_FALSE(task.waiting);
        make(7, 2); // the blocked operation must never switch to this replacement
        resume(); reader.join(); EXPECT_TRUE(status == G_FS_READ_INVALID_FD && count == -1);
        filesystemProcessRemove(42);

        filesystemProcessCreate(42); make(7); waitingPrepared = proceed = false;
        std::thread writer([&] { status = filesystemWrite(&task, 7, nullptr, 1, &count); });
        waitPrepared(); EXPECT_EQ(0, filesystemProcessCloseDescriptor(42, 7));
        resume(); writer.join(); EXPECT_EQ(G_FS_WRITE_INVALID_FD, status);
        filesystemProcessRemove(42);

        filesystemProcessCreate(42); make(7); waitingPrepared = proceed = false;
        std::thread killed([&] {
            EXPECT_THROW(filesystemRead(&task, 7, nullptr, 1, &count), Cancelled);
        });
        waitPrepared(); cancel = true; resume(); killed.join();
        filesystemProcessRemove(42); cancel = false;
        EXPECT_EQ(2, heapBlocks.size()); // only the global process map remains
    }
    hashmapDestroy(filesystemProcessInfo); EXPECT_TRUE(heapBlocks.empty());
}
}
