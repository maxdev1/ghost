#pragma once

#include <array>
#include <string>
#include <algorithm>
using g_fd = int;
Elf64_Shdr section;
bool filesystemReadToMemory(g_fd, uintptr_t, uint8_t* buffer, size_t size) {
    EXPECT_EQ(sizeof(section), size); std::memcpy(buffer, &section, size); return true;
}
void memoryCopy(void* to, void* from, size_t size) { std::memcpy(to, from, size); }
struct ElfSymbolTable {
    g_elf_object object{};
    std::string strings{1, '\0'};
    std::vector<Elf64_Sym> symbols{1};
    std::vector<Elf64_Word> hash;
    alignas(16) uint8_t image[4096]{};
    void add(const std::string& name, uintptr_t value, bool defined = true) {
        Elf64_Sym symbol{};
        symbol.st_name = strings.size(); symbol.st_value = value;
        symbol.st_shndx = defined ? 1 : 0; symbol.st_size = 8;
        strings += name; strings += '\0'; symbols.push_back(symbol);
    }
    void finish(unsigned buckets = 7) {
        hash.assign(2 + buckets + symbols.size(), 0);
        hash[0] = buckets; hash[1] = symbols.size();
        // Independent ABI hash implementation, including high-bit bytes and overflow.
        for(unsigned i = 1; i < symbols.size(); ++i) {
            uint32_t value = 0;
            for(auto p = (const unsigned char*)strings.c_str() + symbols[i].st_name; *p; ++p) {
                value = (value << 4) + *p;
                value ^= (value & 0xf0000000U) >> 24;
                value &= 0x0fffffffU;
            }
            unsigned bucket = 2 + value % buckets;
            hash[2 + buckets + i] = hash[bucket]; hash[bucket] = i;
        }
        object.baseAddress = reinterpret_cast<uintptr_t>(image);
        object.dynamicStringTable = strings.c_str(); object.dynamicStringTableSize = strings.size();
        object.dynamicSymbolTable = symbols.data(); object.dynamicSymbolTableSize = symbols.size();
        object.dynamicSymbolHashTable = hash.data();
    }
};
