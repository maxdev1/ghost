#include "support/fixture.hpp"

namespace
{
static std::map<uint32_t, std::array<uint32_t, 4>> leaves;
static std::vector<uint32_t> msrs;

void processorCpuid(uint32_t leaf, uint32_t* a, uint32_t* b, uint32_t* c, uint32_t* d)
{
    const auto value = leaves[leaf];
    *a = value[0]; *b = value[1]; *c = value[2]; *d = value[3];
}

void processorReadMsr(uint32_t index, uint32_t* low, uint32_t* high)
{
    msrs.push_back(index);
    *low = 123; *high = 1;
}

#include "production/processor_clock_rates.inc"

TEST(ProcessorClock, VmwareRequiresTheVendorAndFrequencyLeaf)
{
    leaves.clear(); msrs.clear();
    leaves[1][2] = 1u << 31;
    leaves[0x40000000] = {0x40000010, 0x61774d56, 0x4d566572, 0x65726177};
    leaves[0x40000010] = {2500000, 100000, 0, 0};
    EXPECT_EQ(2500000u, processorGetVmwareTscKhz());
    EXPECT_EQ(100000000u, processorGetApicFrequencyHz());
    --leaves[0x40000000][0];
    EXPECT_EQ(0u, processorGetVmwareTscKhz());
    EXPECT_EQ(0u, processorGetApicFrequencyHz());
    ++leaves[0x40000000][0];
    leaves[0x40000000][2] = 0;
    EXPECT_EQ(0u, processorGetVmwareTscKhz());
    EXPECT_EQ(0u, processorGetApicFrequencyHz());
    EXPECT_TRUE(msrs.empty());
}

TEST(ProcessorClock, HvFrequencyMsrRequiresInterfacePrivilegeAndImplementation)
{
    leaves.clear(); msrs.clear();
    leaves[1][2] = 1u << 31;
    leaves[0x40000000][0] = 0x40000005;
    leaves[0x40000001][0] = 0x31237648;
    constexpr uint32_t privilege = 1u << 11;
    constexpr uint32_t supported = 1u << 8;
    for(uint32_t eax : {0u, privilege})
        for(uint32_t edx : {0u, supported})
        {
            leaves[0x40000003] = {eax, 0, 0, edx};
            EXPECT_EQ(eax && edx ? (UINT64_C(1) << 32) + 123 : 0, processorGetApicFrequencyHz());
        }
    leaves[0x40000001][0] = 0;
    EXPECT_EQ(0u, processorGetApicFrequencyHz());
    leaves[0x40000001][0] = 0x31237648;
    leaves[1][2] = 0;
    EXPECT_EQ(0u, processorGetApicFrequencyHz());
    EXPECT_EQ(std::vector<uint32_t>({0x40000023}), msrs);
}
}
