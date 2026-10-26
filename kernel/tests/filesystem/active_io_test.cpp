#include "support/fixture.hpp"
#include <condition_variable>

namespace
{
#include "support/filesystem/descriptors.hpp"


TEST(DescriptorLifetime, RetainsActiveIoAfterDetachingProcessMap) {
    filesystemProcessInitialize(); filesystemProcessCreate(42);
    g_file_descriptor* descriptor;
    ASSERT_EQ(0, filesystemProcessCreateDescriptor(42, 1, 1, &descriptor, 7, 123));
    ASSERT_NE(nullptr, descriptor);
    EXPECT_EQ(123, descriptor->offset); filesystemProcessReleaseDescriptor(descriptor);
    mode = 2; int64_t count = -1; int status = -1;
    std::thread reader([&] { status = filesystemRead(&task, 7, nullptr, 1, &count); });
    {
        std::unique_lock<std::mutex> lock(phaseLock);
        phaseChanged.wait(lock, [] { return inRead; });
    }
    filesystemProcessRemove(42);
    EXPECT_TRUE(closedNodes == 0 && !filesystemProcessAcquireDescriptor(42, 7));
    {
        std::lock_guard<std::mutex> lock(phaseLock); allowRead = true; phaseChanged.notify_all();
    }
    reader.join(); EXPECT_TRUE(status == 0 && count == 1 && closedNodes == 1);
    hashmapDestroy(filesystemProcessInfo); EXPECT_TRUE(heapBlocks.empty());
}
}
