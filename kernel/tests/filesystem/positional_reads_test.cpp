#include "support/fixture.hpp"

namespace
{
using g_fd = int;
constexpr int G_FS_READ_SUCCESSFUL = 0, G_FS_LENGTH_SUCCESSFUL = 0;
struct process { int id; } owner{1};
struct task { struct process* process; } current{&owner};
struct node {} file;
struct file_descriptor { int nodeId; uint64_t offset; struct node* node; } descriptor{1, 0x777, &file};

task* taskingGetCurrentTask() { return &current; }
std::atomic<int> pins{0};
file_descriptor* filesystemProcessAcquireDescriptor(int, int) { ++pins; return &descriptor; }
void filesystemProcessReleaseDescriptor(file_descriptor* d) { if(d) --pins; }
node* filesystemGetNode(int) { return &file; }
int filesystemGetLength(node*, uint64_t* length) { *length = 4096; return G_FS_LENGTH_SUCCESSFUL; }
int filesystemRead(node*, uint8_t* buffer, uint64_t offset, uint64_t length, int64_t* read) {
    *read = length > 7 ? 7 : length;
    std::this_thread::yield();
    for(int64_t i = 0; i < *read; ++i) buffer[i] = uint8_t(offset + i);
    return G_FS_READ_SUCCESSFUL;
}

#include "production/filesystemReadToMemory.inc"

TEST(PositionalReads, ConcurrentReadsPreserveDescriptorOffsetAndRejectOutOfBounds) {
    std::vector<std::thread> threads;
    for(int core = 0; core < 16; ++core) threads.emplace_back([core] {
        for(int trial = 0; trial < 100; ++trial) {
            uint8_t buffer[64];
            size_t offset = core * 131 + trial;
            EXPECT_TRUE(filesystemReadToMemory(1, offset, buffer, sizeof(buffer)));
            for(size_t i = 0; i < sizeof(buffer); ++i) EXPECT_EQ(uint8_t(offset + i), buffer[i]);
        }
    });
    for(auto& thread : threads) thread.join();
    EXPECT_EQ(0x777, descriptor.offset);
    uint8_t buffer[64];
    EXPECT_FALSE(filesystemReadToMemory(1, 4097, buffer, 1));
    EXPECT_FALSE(filesystemReadToMemory(1, 4090, buffer, 64));
    EXPECT_FALSE(filesystemReadToMemory(1, UINTPTR_MAX, buffer, 64));
    EXPECT_EQ(0, pins.load());
}
}
