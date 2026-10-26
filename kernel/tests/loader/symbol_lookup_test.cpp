#include "support/fixture.hpp"

namespace
{
#include "support/loader/symbol_fixture.hpp"

TEST(ElfSymbolLookup, MatchesIndependentOracleForDuplicateDefinitionsAndCollisions) {
    ElfSymbolTable f;
    for(int i = 0; i < 400; ++i) {
        f.add("symbol_" + std::to_string(i), i * 8, i % 11 != 0);
        if(i % 7 == 0) f.add("symbol_" + std::to_string(i), i * 8 + 1);
    }
    f.add("a very long symbol name that exercises hash overflow", 9000);
    f.add(std::string("high_") + char(0xfe), 9001);
    f.finish();
    EXPECT_EQ(0, _elfObjectSymbolHash(""));
    EXPECT_EQ(0x77905a6, _elfObjectSymbolHash("printf"));
    for(const auto& symbol : f.symbols) {
        const char* name = f.strings.c_str() + symbol.st_name;
        Elf64_Sym *first = nullptr, *last = nullptr;
        for(auto& candidate : f.symbols) {
            if(candidate.st_shndx && !std::strcmp(f.strings.c_str() + candidate.st_name, name)) {
                if(!first) first = &candidate;
                last = &candidate;
            }
        }
        auto hash = _elfObjectSymbolHash(name);
        EXPECT_EQ(first, _elfObjectFindSymbol(&f.object, name, hash, false));
        EXPECT_EQ(last, _elfObjectFindSymbol(&f.object, name, hash, true));
    }
    EXPECT_FALSE(_elfObjectFindSymbol(&f.object, "absent", _elfObjectSymbolHash("absent"), false));
    EXPECT_TRUE(heapBlocks.empty()); // Lookup allocates no kernel heap objects or copied names.
}
}
