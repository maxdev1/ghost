#include "support/fixture.hpp"

namespace
{
#include "support/ipc/pipe_fixture.hpp"

TEST(PipeTransfers, PreservesWrappedStreamDataAndAtomicWriteBounds) {
    int id; EXPECT_EQ(0, pipeCreate(&id, true));
    published->referencesRead = published->referencesWrite = 1;
    std::vector<uint8_t> source(32768), result(32768);
    for(size_t i = 0; i < source.size(); ++i) source[i] = i * 37;
    int64_t written = -1, read = -1;
    EXPECT_TRUE(pipeWrite(id, source.data(), 0, 12288, &written) == 0 && written == 12288);
    EXPECT_TRUE(pipeRead(id, result.data(), 0, 8192, &read) == 0 && read == 8192);
    EXPECT_FALSE(std::memcmp(result.data(), source.data(), read));
    EXPECT_TRUE(pipeWrite(id, source.data() + 12288, 0, 10240, &written) == 0 && written == 10240);
    EXPECT_TRUE(pipeRead(id, result.data(), 0, result.size(), &read) == 0 && read == 14336);
    EXPECT_FALSE(std::memcmp(result.data(), source.data() + 8192, read));
    EXPECT_TRUE(pipeWrite(id, source.data(), 0, 15872, &written) == 0 && written == 15872);
    EXPECT_TRUE(pipeWrite(id, source.data(), 0, 1024, &written) == G_FS_WRITE_BUSY && written == 0);
    EXPECT_TRUE(pipeWrite(id, source.data(), 0, 1025, &written) == 0 && written == 512);
    EXPECT_TRUE(pipeWrite(id, source.data(), 0, 2048, &written) == G_FS_WRITE_BUSY && written == 0);
    EXPECT_TRUE(pipeRead(id, result.data(), 0, result.size(), &read) == 0 && read == 16384);
    EXPECT_TRUE(pipeRead(id, result.data(), 0, 1, &read) == G_FS_READ_BUSY && read == 0);
    published->referencesWrite = 0;
    EXPECT_TRUE(pipeRead(id, result.data(), 0, 1, &read) == 0 && read == 0);
    published->referencesRead = 0;
    EXPECT_TRUE(pipeWrite(id, source.data(), 0, 16, &written) == G_FS_WRITE_ERROR && written == 0);
    EXPECT_TRUE(wakeups && published->lock.depth == 0);
    destroy();
}
}
