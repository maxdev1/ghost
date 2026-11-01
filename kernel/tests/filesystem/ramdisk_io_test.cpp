#include "support/fixture.hpp"

namespace
{
using g_ramdisk_entry_type = uint8_t;
using g_ramdisk_id = uint32_t;
using g_fs_read_status = int;
using g_fs_write_status = int;
using g_fs_open_status = int;
constexpr int G_FS_READ_SUCCESSFUL = 0, G_FS_READ_ERROR = 1;
constexpr int G_FS_WRITE_SUCCESSFUL = 0, G_FS_WRITE_ERROR = 1;
constexpr int G_FS_OPEN_SUCCESSFUL = 0, G_FS_OPEN_ERROR = 1;
#include "production/ramdisk_entry.inc"

struct g_fs_node { uint64_t physicalId; } node{1};
g_ramdisk_entry entry;
bool allocationFailure;
std::unordered_set<void*> allocations;

g_ramdisk_entry* ramdiskFindById(uint64_t id) { return id == 1 ? &entry : nullptr; }
void* heapAllocate(uint32_t size)
{
	if(allocationFailure) return nullptr;
	void* data = malloc(size);
	if(data) allocations.insert(data);
	return data;
}
void heapFree(void* data)
{
	ASSERT_EQ(1u, allocations.erase(data));
	free(data);
}
void* memoryCopy(void* target, const void* source, int32_t size) { return memcpy(target, source, size); }
void* memorySetBytes(void* target, uint8_t value, int32_t size) { return memset(target, value, size); }

#include "production/ramdisk_io.inc"

class RamdiskIo : public ::testing::Test
{
protected:
	uint8_t original[4]{'a', 'b', 'c', 'd'};
	void SetUp() override
	{
		entry = {};
		entry.data = original;
		entry.dataSize = sizeof(original);
		entry.dataOnRamdisk = true;
		allocationFailure = false;
	}
	void TearDown() override
	{
		EXPECT_EQ(G_FS_OPEN_SUCCESSFUL, filesystemRamdiskDelegateTruncate(&node));
		EXPECT_TRUE(allocations.empty());
	}
	void ramdiskTestWrite(uint64_t offset, std::vector<uint8_t> bytes)
	{
		int64_t count = -1;
		ASSERT_EQ(G_FS_WRITE_SUCCESSFUL,
		          filesystemRamdiskDelegateWrite(&node, bytes.data(), offset, bytes.size(), &count));
		ASSERT_EQ(int64_t(bytes.size()), count);
		EXPECT_TRUE(std::equal(bytes.begin(), bytes.end(), entry.data + offset));
	}
};

TEST_F(RamdiskIo, TruncatesOriginalStorageWithoutFreeingBootMemory)
{
	ASSERT_EQ(G_FS_OPEN_SUCCESSFUL, filesystemRamdiskDelegateTruncate(&node));
	EXPECT_EQ(0u, entry.dataSize);
	EXPECT_EQ(nullptr, entry.data);
	EXPECT_FALSE(entry.dataOnRamdisk);
	EXPECT_EQ('a', original[0]);
	int64_t count = -1;
	EXPECT_EQ(G_FS_READ_SUCCESSFUL, filesystemRamdiskDelegateRead(&node, nullptr, 0, 1, &count));
	EXPECT_EQ(0, count);
	ramdiskTestWrite(0, {'x'});
}

TEST_F(RamdiskIo, GrowsEmptyAndTinyOriginalFilesForAppend)
{
	for(uint32_t size = 0; size < sizeof(original); ++size)
	{
		entry.dataSize = size;
		ramdiskTestWrite(size, std::vector<uint8_t>(16384, 0xa5));
		EXPECT_EQ(size + 16384, entry.dataSize);
		EXPECT_TRUE(std::equal(original, original + size, entry.data));
		ASSERT_EQ(G_FS_OPEN_SUCCESSFUL, filesystemRamdiskDelegateTruncate(&node));
		entry.data = original;
		entry.dataOnRamdisk = true;
	}
}

TEST_F(RamdiskIo, OverwritingAPrefixPreservesTheRestOfTheFile)
{
	ramdiskTestWrite(0, {'x'});
	EXPECT_EQ(4u, entry.dataSize);
	EXPECT_EQ('b', entry.data[1]);
	EXPECT_EQ('d', entry.data[3]);
	EXPECT_EQ('a', original[0]);
	ramdiskTestWrite(1, {'y'});
	EXPECT_EQ(4u, entry.dataSize);
	EXPECT_EQ('d', entry.data[3]);
}

TEST_F(RamdiskIo, GrowsOwnedStorageAndZeroesGaps)
{
	ramdiskTestWrite(0, std::vector<uint8_t>(64, 0x12));
	ramdiskTestWrite(96, std::vector<uint8_t>(16384, 0x34));
	EXPECT_TRUE(std::all_of(entry.data, entry.data + 64, [](uint8_t c) { return c == 0x12; }));
	EXPECT_TRUE(std::all_of(entry.data + 64, entry.data + 96, [](uint8_t c) { return c == 0; }));
	EXPECT_EQ(1u, allocations.size());
}

TEST_F(RamdiskIo, AllocationFailurePreservesTheExistingData)
{
	allocationFailure = true;
	uint8_t byte = 'x';
	int64_t count = 0;
	EXPECT_EQ(G_FS_WRITE_ERROR, filesystemRamdiskDelegateWrite(&node, &byte, 4, 1, &count));
	EXPECT_EQ(-1, count);
	EXPECT_EQ(original, entry.data);
	EXPECT_EQ(4u, entry.dataSize);
	EXPECT_TRUE(entry.dataOnRamdisk);
}

TEST_F(RamdiskIo, RejectsOverflowingWritesWithoutAllocating)
{
	uint8_t byte = 0;
	int64_t count;
	for(auto offset : {uint64_t(INT32_MAX), UINT64_MAX})
	{
		EXPECT_EQ(G_FS_WRITE_ERROR, filesystemRamdiskDelegateWrite(&node, &byte, offset, 1, &count));
		EXPECT_EQ(-1, count);
	}
	EXPECT_TRUE(allocations.empty());
	EXPECT_EQ(original, entry.data);
}

TEST_F(RamdiskIo, ZeroLengthWriteDoesNotChangeStorageOrFileSize)
{
	int64_t count = -1;
	EXPECT_EQ(G_FS_WRITE_SUCCESSFUL, filesystemRamdiskDelegateWrite(&node, nullptr, UINT64_MAX, 0, &count));
	EXPECT_EQ(0, count);
	EXPECT_EQ(original, entry.data);
	EXPECT_EQ(4u, entry.dataSize);
	EXPECT_TRUE(allocations.empty());
}

TEST_F(RamdiskIo, ReadsAtAndBeyondEofAndWithOversizedLengths)
{
	uint8_t buffer[8]{};
	int64_t count;
	for(uint64_t offset : {uint64_t(4), uint64_t(9), UINT64_MAX})
	{
		EXPECT_EQ(G_FS_READ_SUCCESSFUL, filesystemRamdiskDelegateRead(&node, buffer, offset, 8, &count));
		EXPECT_EQ(0, count);
	}
	EXPECT_EQ(G_FS_READ_SUCCESSFUL, filesystemRamdiskDelegateRead(&node, buffer, 2, UINT64_MAX, &count));
	EXPECT_EQ(2, count);
	EXPECT_EQ('c', buffer[0]);
	EXPECT_EQ('d', buffer[1]);
}
}
