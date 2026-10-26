#include "support/fixture.hpp"

namespace
{
#include "support/loader/symbol_fixture.hpp"

TEST(ElfSymbolLookup, BoundsTraversalOfMissingCyclicAndOutOfRangeHashChains) {
    ElfSymbolTable f; f.add("present", 1); f.finish(1);
    EXPECT_FALSE(_elfObjectFindSymbol(&f.object, "missing", 0, false));
    f.hash[4] = 1; // Cyclic chain.
    EXPECT_FALSE(_elfObjectFindSymbol(&f.object, "missing", 0, false));
    f.hash[2] = 9999; // Out-of-range bucket index.
    EXPECT_FALSE(_elfObjectFindSymbol(&f.object, "missing", 0, false));
    f.hash[0] = 0;
    EXPECT_FALSE(_elfObjectFindSymbol(&f.object, "present", 0, false));
    f.object.dynamicSymbolHashTable = nullptr;
    EXPECT_FALSE(_elfObjectFindSymbol(&f.object, "present", 0, false));
}
}
