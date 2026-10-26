#include "support/fixture.hpp"

namespace
{
#include "support/tracked_heap.hpp"

using g_tid = int;
using g_message_send_status = int;
constexpr int G_MESSAGE_MAXIMUM_MESSAGE_LENGTH = 2048;
constexpr int G_MESSAGE_SEND_STATUS_EXCEEDS_MAXIMUM = 4, G_MESSAGE_SEND_STATUS_SUCCESSFUL = 1;
struct g_message_header { int sender, transaction; size_t length; };
#define G_MESSAGE_CONTENT(m) ((uint8_t*)(m) + sizeof(g_message_header))
struct g_message_topic { g_mutex lock; int nextTransaction, waitersReceive; } topic;
int creates, wakes;
g_message_header* published;
g_message_topic* _messageTopicsGetOrCreate(const char*) { ++creates; return &topic; }
void _messageTopicsAddToTail(g_message_topic*, g_message_header* message) { published = message; }
void waitQueueWake(int*) { ++wakes; }
void memoryCopy(void* out, const void* in, size_t size) { std::memcpy(out, in, size); }

#include "production/messageTopicsPost.inc"

TEST(MessageTopics, RejectsOversizedPayloadsAndPublishesAnOwnedCopy) {
    EXPECT_EQ(G_MESSAGE_SEND_STATUS_EXCEEDS_MAXIMUM, messageTopicsPost("topic", 1, nullptr, UINT32_MAX));
    EXPECT_EQ(G_MESSAGE_SEND_STATUS_EXCEEDS_MAXIMUM, messageTopicsPost("topic", 1, nullptr, 2049));
    EXPECT_TRUE(creates == 0 && heapBlocks.empty());
    uint8_t data[2048];
    for(size_t i = 0; i < sizeof(data); ++i) data[i] = static_cast<uint8_t>(i * 37);
    ASSERT_EQ(G_MESSAGE_SEND_STATUS_SUCCESSFUL, messageTopicsPost("topic", 1, data, sizeof(data)));
    ASSERT_NE(nullptr, published);
    EXPECT_EQ(1, creates);
    EXPECT_EQ(1, wakes);
    EXPECT_EQ(sizeof(data), published->length);
    EXPECT_EQ(1, published->sender);
    EXPECT_EQ(0, published->transaction);
    EXPECT_EQ(1, topic.nextTransaction);
    EXPECT_EQ(0, std::memcmp(G_MESSAGE_CONTENT(published), data, sizeof(data)));
    data[0] ^= 0xff;
    EXPECT_NE(data[0], G_MESSAGE_CONTENT(published)[0]);
    EXPECT_EQ(0, topic.lock.depth);
    heapFree(published); EXPECT_TRUE(heapBlocks.empty());
}
}
