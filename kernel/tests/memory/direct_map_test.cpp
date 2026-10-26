#include "support/fixture.hpp"

namespace
{
#include "production/direct_map_offset.inc"
#include "production/physical_to_virtual.inc"

TEST(DirectMap, AddsHigherHalfOffsetWithoutTruncating64BitPhysicalAddresses)
{
    EXPECT_EQ(UINT64_C(0xffff800000000000), G_MEM_PHYS_TO_VIRT(0));
    EXPECT_EQ(UINT64_C(0xffff800000001000), G_MEM_PHYS_TO_VIRT(0x1000));
    EXPECT_EQ(UINT64_C(0xffff800100000000), G_MEM_PHYS_TO_VIRT(UINT64_C(0x100000000)));
    EXPECT_EQ(UINT64_C(0xffff800123456000), G_MEM_PHYS_TO_VIRT(UINT64_C(0x123456000)));
}
}
