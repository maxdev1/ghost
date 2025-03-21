/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2015, Max Schlüssel <lokoxe@gmail.com>                     *
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

#ifndef GHOST_API_ELF64
#define GHOST_API_ELF64

#include "common.h"
#include "stdint.h"

__BEGIN_C

/**
 * ELF64 types
*/
typedef uint64_t Elf64_Addr;
typedef uint64_t Elf64_Off;
typedef uint16_t Elf64_Half;
typedef uint32_t Elf64_Word;
typedef int32_t Elf64_Sword;
typedef uint64_t Elf64_Xword;
typedef int64_t Elf64_Sxword;

/**
 * Ident header
 */
#define EI_MAG0			0
#define EI_MAG1			1
#define EI_MAG2			2
#define EI_MAG3			3
#define EI_CLASS		4
#define EI_DATA			5
#define EI_VERSION		6
#define EI_PAD			7
#define EI_NIDENT		16

#define ELFMAG0			0x7F
#define ELFMAG1			'E'
#define ELFMAG2			'L'
#define ELFMAG3			'F'

#define ELFCLASS32		1
#define ELFCLASS64		2

#define ELFDATA2LSB		1
#define ELFDATA2MSB		2

#define ELFOSABI_SYSV	0
#define ELFOSABI_HPUX	1
#define ELFOSABI_STANDALONE	255

/**
 * ELF header
 */
#define ET_NONE			0
#define ET_REL			1
#define ET_EXEC			2
#define ET_DYN			3
#define ET_CORE			4
#define ET_LOOS			0xFE00
#define ET_HIOS			0xFEFF
#define ET_LOPROC		0xFF00
#define ET_HIPROC		0xFFFF

#define EM_NONE			0
#define EM_M32			1
#define EM_SPARC		2
#define EM_386			3
#define EM_68K			4
#define EM_88K			5
#define EM_860			6
#define EM_MIPS			8
#define EM_X86_64		62

#define EV_NONE			0
#define EV_CURRENT		1

typedef struct
{
	uint8_t e_ident[EI_NIDENT]; // Ident structure
	Elf64_Half e_type; // Type
	Elf64_Half e_machine; // Target architecture
	Elf64_Word e_version; // EV_CURRENT for standard ELF files
	Elf64_Addr e_entry; // Entry's virtual address
	Elf64_Off e_phoff; // Offset to program header table
	Elf64_Off e_shoff; // Offset to section header table
	Elf64_Word e_flags; // Architecture-specific flags
	Elf64_Half e_ehsize; // Size of ELF header in bytes
	Elf64_Half e_phentsize; // Size of one entry in program header table
	Elf64_Half e_phnum; // Number of entries in program header table
	Elf64_Half e_shentsize; // Size of one entry in section header table
	Elf64_Half e_shnum; // Number of entries in section header table
	Elf64_Half e_shstrndx; // Section header table index of section name string table
} __attribute__((packed)) Elf64_Ehdr;

/**
 * Program header
 */
#define PT_NULL			0
#define PT_LOAD			1
#define PT_DYNAMIC		2
#define PT_INTERP		3
#define PT_NOTE			4
#define PT_SHLIB		5
#define PT_PHDR			6
#define PT_TLS			7
#define PT_LOPROC		0x70000000
#define PT_HIPROC		0x7FFFFFFF

#define PF_X			1
#define PF_W			2
#define PF_R			4
#define PF_MASKOS		0x0FF00000
#define PF_MASKPROC		0xF0000000

typedef struct
{
	Elf64_Word p_type; // Type of the segment
	Elf64_Word p_flags; // Segment flags
	Elf64_Off p_offset; // Offset of the segment in the binary file
	Elf64_Addr p_vaddr; // Virtual address
	Elf64_Addr p_paddr; // Not relevant for System V
	Elf64_Xword p_filesz; // Size of the segment in the binary file
	Elf64_Xword p_memsz; // Size of the segment in memory
	Elf64_Xword p_align; // Alignment information
} __attribute__((packed)) Elf64_Phdr;

/**
 * ELF dynamic section structure
 */
typedef struct
{
	Elf64_Sxword d_tag; // Controls interpretation of d_un, see DT_*
	union
	{
		Elf64_Xword d_val;
		Elf64_Addr d_ptr;
	} d_un;
} __attribute__((packed)) Elf64_Dyn;

#define DT_NULL				0
#define DT_NEEDED			1
#define DT_PLTRELSZ			2
#define DT_PLTGOT			3
#define DT_HASH				4
#define DT_STRTAB			5
#define DT_SYMTAB			6
#define DT_RELA				7
#define DT_RELASZ			8
#define DT_RELAENT			9
#define DT_STRSZ			10
#define DT_SYMENT			11
#define DT_INIT				12
#define DT_FINI				13
#define DT_SONAME			14
#define DT_RPATH			15
#define DT_SYMBOLIC			16
#define DT_REL				17
#define DT_RELSZ			18
#define DT_RELENT			19
#define DT_PLTREL			20
#define DT_DEBUG			21
#define DT_TEXTREL			22
#define DT_JMPREL			23
#define DT_BIND_NOW			24
#define DT_INIT_ARRAY		25
#define DT_FINI_ARRAY		26
#define DT_INIT_ARRAYSZ		27
#define DT_FINI_ARRAYSZ		28
#define DT_RUNPATH			29
#define DT_FLAGS			30
#define DT_ENCODING			32
#define DT_PREINIT_ARRAY	32
#define DT_PREINIT_ARRAYSZ	33
#define DT_SYMTAB_SHNDX		34
#define DT_NUM				35
#define DT_LOPROC			0x70000000
#define DT_HIPROC			0x7fffffff


/**
 * ELF symbol table
 */
typedef struct
{
	Elf64_Word st_name; // Index to symbol string name
	uint8_t st_info; // Type and binding attributes
	uint8_t st_other; // Visibility
	Elf64_Half st_shndx; // Section header table index
	Elf64_Addr st_value; // Value of associated symbol
	Elf64_Xword st_size; // Size of the symbol
} Elf64_Sym;

#define ELF64_ST_BIND(i)	((i) >> 4)
#define ELF64_ST_TYPE(i)	((i) & 0xf)
#define ELF64_ST_INFO(b, t)	(((b) << 4) + ((t) & 0xf)

#define STN_UNDEF	0

#define STB_LOCAL	0
#define STB_GLOBAL	1
#define STB_WEAK	2
#define STB_LOPROC	13
#define STB_HIPROC	15

#define STT_NOTYPE	0
#define STT_OBJECT	1
#define STT_FUNC	2
#define STT_SECTION	3
#define STT_FILE	4
#define STT_LOPROC	13
#define STT_HIPROC	15

/**
 * ELF section header
 */
typedef struct
{
	Elf64_Word sh_name;
	Elf64_Word sh_type;
	Elf64_Xword sh_flags;
	Elf64_Addr sh_addr;
	Elf64_Off sh_offset;
	Elf64_Xword sh_size;
	Elf64_Word sh_link;
	Elf64_Word sh_info;
	Elf64_Xword sh_addralign;
	Elf64_Xword sh_entsize;
} Elf64_Shdr;

#define SHT_NULL		0
#define SHT_PROGBITS	1
#define SHT_SYMTAB		2
#define SHT_STRTAB		3
#define SHT_RELA		4
#define SHT_HASH		5
#define SHT_DYNAMIC		6
#define SHT_NOTE		7
#define SHT_NOBITS		8
#define SHT_REL			9
#define SHT_SHLIB		10
#define SHT_DYNSYM		11
#define SHT_LOOS		0x60000000
#define SHT_HIOS		0x6fffffff
#define SHT_LOPROC		0x70000000
#define SHT_HIPROC		0x7fffffff
#define SHT_LOUSER		0x80000000
#define SHT_HIUSER		0xffffffff

/**
 * Relocation header entry
 */
typedef struct
{
	Elf64_Addr r_offset;
	Elf64_Xword r_info;
} Elf64_Rel;

typedef struct
{
	Elf64_Addr r_offset;
	Elf64_Xword r_info;
	Elf64_Sxword r_addend;
} Elf64_Rela;

#define ELF64_R_SYM(i)		((i) >> 32)
#define ELF64_R_TYPE(i)		((i) & 0xFFFFFFFFL)
#define ELF64_R_INFO(s, t)	(((s) << 32) + ((t) & 0xFFFFFFFFL))

#define R_X86_64_NONE		0
#define R_X86_64_64			1
#define R_X86_64_PC32		2
#define R_X86_64_GOT32		3
#define R_X86_64_PLT32		4
#define R_X86_64_COPY		5
#define R_X86_64_GLOB_DAT	6
#define R_X86_64_JMP_SLOT	7
#define R_X86_64_RELATIVE	8
#define R_X86_64_GOTPCREL	9
#define R_X86_64_32			10
#define R_X86_64_32S		11

// Thread-local storage related relocation types
// TODO Check these
#define R_X86_64_TLS_GD_PLT		12
#define R_X86_64_TLS_LDM_PLT	13
#define R_X86_64_TLS_TPOFF		14
#define R_X86_64_TLS_IE			15
#define R_X86_64_TLS_GOTIE		16
#define R_X86_64_TLS_LE			17
#define R_X86_64_TLS_GD			18
#define R_X86_64_TLS_LDM		19
#define R_X86_64_TLS_LDO_32		32
#define R_X86_64_TLS_DTPMOD32	35
#define R_X86_64_TLS_DTPOFF32	36

__END_C

#endif
