#include "support/fixture.hpp"

namespace
{
uint32_t sleepDivisor = 238;
uint8_t control;
unsigned polls;
std::vector<uint8_t> countBytes;
std::vector<uint8_t> gateWrites;

uint8_t ioPortReadByte(uint16_t port)
{
    EXPECT_EQ(0x61, port);
    if((control & 1) && ++polls == 3)
        control |= 0x30; // OUT2 goes high and another status bit changes.
    return control;
}

void ioPortWriteByte(uint16_t port, uint8_t value)
{
    if(port == 0x42)
    {
        countBytes.push_back(value);
        control &= ~0x20; // A new mode-0 count clears OUT2.
        return;
    }
    EXPECT_EQ(0x61, port);
    gateWrites.push_back(value);
    control = value;
}

#include "production/pit_perform_sleep.inc"

TEST(PitSleep, CompletesEachDelayAndLeavesChannelTwoGatedOff)
{
    control = 0x02;
    for(int run = 0; run < 2; ++run)
    {
        polls = 0;
        gateWrites.clear();
        countBytes.clear();
        pitPerformSleep();
        EXPECT_EQ((std::vector<uint8_t>{238, 0}), countBytes);
        ASSERT_EQ(3u, gateWrites.size());
        EXPECT_EQ(0, gateWrites[0] & 1);
        EXPECT_EQ(1, gateWrites[1] & 1);
        EXPECT_EQ(0x32, gateWrites[2]);
        EXPECT_EQ(0x32, control);
    }
}
}
