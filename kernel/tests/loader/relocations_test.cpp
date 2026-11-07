/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2026, Max Schlüssel <lokoxe@gmail.com>                     *
 *                                                                           *
 *  This program is free software: you can redistribute it and/or modify     *
 *  it under the terms of the GNU General Public License as published by     *
 *  the Free Software Foundation, either version 3 of the License, or        *
 *  (at your option) any later version.                                      *
 *                                                                           *
 *  This program is distributed in the hope that it will be useful,          *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *  GNU General Public License for more details.                             *
 *                                                                           *
 *  You should have received a copy of the GNU General Public License        *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
 *                                                                           *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "support/fixture.hpp"

namespace
{
#include "support/loader/symbol_fixture.hpp"
#include "production/elfObjectApplyRelocations.inc"

TEST(ElfRelocations, ResolvesGlobalCopyWeakTlsAndSymbollessRelativeRelocations) {
    ElfSymbolTable root, first, last;
    root.add("duplicate", 64); root.add("duplicate", 72);
    root.add("copy", 80, false); root.add("tls", 0, false);
    root.add("weak_missing", 0, false); root.symbols.back().st_info = STB_WEAK << 4;
    first.add("duplicate", 96); first.add("copy", 104); first.add("tls", 24);
    last.add("copy", 112); last.add("copy", 120);
    root.finish(); first.finish(); last.finish();
    first.object.parent = last.object.parent = &root.object;
    root.object.symbolLoadOrderNext = &first.object; first.object.symbolLoadOrderNext = &last.object;
    root.object.symbolLookupOrderList = &last.object;
    last.object.symbolLookupOrderListNext = &first.object; first.object.symbolLookupOrderListNext = &root.object;
    first.object.id = 2; first.object.tlsPart.offset = 256; root.object.tlsMaster.userThreadOffset = 512;
    *reinterpret_cast<uint64_t*>(last.image + 112) = 111;
    *reinterpret_cast<uint64_t*>(last.image + 120) = 222;
    auto slots = reinterpret_cast<uint64_t*>(root.image + 256);
    auto entries = reinterpret_cast<Elf64_Rela*>(root.image + 512);
    unsigned types[] = {R_X86_64_GLOB_DAT, R_X86_64_COPY, R_X86_64_DTPMOD64,
                        R_X86_64_TPOFF64, R_X86_64_GLOB_DAT, R_X86_64_RELATIVE};
    unsigned symbols[] = {1, 3, 4, 4, 5, 0};
    for(unsigned i = 0; i < 6; ++i) {
        slots[i] = 13;
        entries[i] = Elf64_Rela{};
        entries[i].r_offset = 256 + i * 8;
        entries[i].r_info = (uint64_t(symbols[i]) << 32) | types[i];
    }
    section.sh_type = SHT_RELA; section.sh_addr = 512; section.sh_size = 6 * sizeof(Elf64_Rela);
    root.object.header.e_shnum = 1; root.object.header.e_shentsize = sizeof(section);
    elfObjectApplyRelocations(0, &root.object);
    EXPECT_EQ(root.object.baseAddress + 64, slots[0]); // First definition, first-loaded object.
    EXPECT_EQ(222, slots[1]); // Last definition, last-loaded object for COPY.
    EXPECT_EQ(uint64_t(256) - uint64_t(512), slots[2]);
    EXPECT_EQ(uint64_t(256) - uint64_t(512) + 24, slots[3]);
    EXPECT_EQ(0, slots[4]); // Missing weak symbol.
    EXPECT_EQ(root.object.baseAddress + 13, slots[5]);
    // RELATIVE relocations need no symbol/string/hash table at all.
    root.object.dynamicSymbolTable = nullptr; root.object.dynamicStringTable = nullptr;
    root.object.dynamicSymbolHashTable = nullptr;
    section.sh_addr += 5 * sizeof(Elf64_Rela); section.sh_size = sizeof(Elf64_Rela);
    slots[5] = 21;
    elfObjectApplyRelocations(0, &root.object);
    EXPECT_EQ(root.object.baseAddress + 21, slots[5]);
    EXPECT_TRUE(heapBlocks.empty());
}

TEST(ElfRelocations, LocalDynamicTlsWithoutSymbolsPreservesModuleBaseAndLinkedOffset)
{
	ElfSymbolTable root, library;
	root.finish();
	library.finish();
	library.object.parent = &root.object;
	root.object.tlsMaster.userThreadOffset = 16;
	library.object.tlsPart.offset = 32;
	auto slots = reinterpret_cast<uint64_t*>(library.image + 256);
	auto entries = reinterpret_cast<Elf64_Rela*>(library.image + 512);
	slots[0] = 0;
	slots[1] = 128;
	entries[0] = Elf64_Rela{};
	entries[0].r_offset = 256;
	entries[0].r_info = R_X86_64_DTPMOD64;
	section.sh_type = SHT_RELA;
	section.sh_addr = 512;
	section.sh_size = sizeof(Elf64_Rela);
	library.object.header.e_shnum = 1;
	library.object.header.e_shentsize = sizeof(section);
	library.object.dynamicSymbolTable = nullptr;
	library.object.dynamicStringTable = nullptr;
	library.object.dynamicSymbolHashTable = nullptr;

	elfObjectApplyRelocations(0, &library.object);
	EXPECT_EQ(16u, slots[0]);
	EXPECT_EQ(128u, slots[1]);

	entries[0].r_info = R_X86_64_DTPOFF64;
	entries[0].r_addend = 48;

	elfObjectApplyRelocations(0, &library.object);
	EXPECT_EQ(48u, slots[0]);

	entries[0].r_info = R_X86_64_TPOFF64;
	entries[0].r_addend = 4;
	library.object.tlsPart.offset = 0;

	elfObjectApplyRelocations(0, &library.object);
	EXPECT_EQ(uint64_t(0) - 16 + 4, slots[0]);
}

}
