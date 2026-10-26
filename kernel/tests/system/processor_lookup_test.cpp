#include "support/fixture.hpp"

namespace
{
#include "production/processor_layout.inc"

static g_processor* processors;
static uint32_t processorsAvailable;
uint32_t activeCore;
uint32_t processorGetCurrentId() { return activeCore; }
bool lapicIsAvailable() { return true; }
uint32_t lapicReadId() { return 0; }
void* heapAllocate(size_t size) { auto p = std::malloc(size); std::memset(p, 0xa5, size); return p; }
void* heapAllocateClear(size_t size) { return std::calloc(1, size); }

#include "production/processor_lookup_and_add.inc"

TEST(ProcessorLookup, ResolvesLogicalIdsInReverseRegistrationOrder) {
    for(int i = 0; i < 16; ++i) processorAdd(i, i);
    for(activeCore = 0; activeCore < 16; ++activeCore) {
        auto current = _processorGetCurrent();
        EXPECT_TRUE(current && current->id == activeCore);
        EXPECT_TRUE(current->bsp == (activeCore == 0) && !current->sseReady);
    }
    while(processors) { auto next = processors->next; std::free(processors); processors = next; }
}
}
