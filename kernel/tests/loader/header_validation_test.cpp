#include "support/fixture.hpp"

namespace
{
#include "support/loader/validation.hpp"

TEST(ElfHeaders, RejectsMalformedHeaderSizesAndTableOffsets)
{
    Elf64_Ehdr good{};
    good.e_ident[EI_MAG0] = ELFMAG0; good.e_ident[EI_MAG1] = ELFMAG1;
    good.e_ident[EI_MAG2] = ELFMAG2; good.e_ident[EI_MAG3] = ELFMAG3;
    good.e_ident[EI_CLASS] = ELFCLASS64; good.e_ident[EI_DATA] = ELFDATA2LSB;
    good.e_ident[EI_VERSION] = good.e_version = EV_CURRENT;
    good.e_type = ET_EXEC; good.e_machine = EM_X86_64;
    good.e_ehsize = sizeof(Elf64_Ehdr); good.e_phentsize = sizeof(Elf64_Phdr);
    good.e_phnum = good.e_shnum = 1; good.e_shentsize = sizeof(Elf64_Shdr);
    EXPECT_EQ(0, elfValidateHeader(&good, true));
    auto bad = good; bad.e_shentsize = 65535; EXPECT_NE(0, elfValidateHeader(&bad, true));
    bad = good; bad.e_shentsize = 0; EXPECT_NE(0, elfValidateHeader(&bad, true));
    bad = good; bad.e_phoff = UINT64_MAX; EXPECT_NE(0, elfValidateHeader(&bad, true));
    bad = good; bad.e_phentsize = 0; EXPECT_NE(0, elfValidateHeader(&bad, true));
    bad = good; bad.e_ehsize = 0; EXPECT_NE(0, elfValidateHeader(&bad, true));
}
}
