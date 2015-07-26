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

#ifndef __ELF32_LOADER__
#define __ELF32_LOADER__

#include "loader.hpp"
#include <ghost/elf32.h>

/**
 *
 */
enum elf32_validation_status_t {
	ELF32_VALIDATION_SUCCESSFUL,
	ELF32_VALIDATION_NOT_ELF,
	ELF32_VALIDATION_NOT_EXECUTABLE,
	ELF32_VALIDATION_NOT_I386,
	ELF32_VALIDATION_NOT_32BIT,
	ELF32_VALIDATION_NOT_LITTLE_ENDIAN,
	ELF32_VALIDATION_NOT_STANDARD_ELF
};

/**
 *
 */
class elf32_loader_t: public loader_t {
public:
	elf32_loader_t(g_process_creation_identifier target, g_fd file) :
			loader_t(target, file) {
	}
	virtual ~elf32_loader_t() {
	}

	virtual loader_status_t load(uintptr_t* out_entry_addr);

	static loader_status_t read_and_validate_elf_header(g_fd file,
			elf32_ehdr* hdrBuf);
	static loader_status_t check_for_elf_binary_and_reset(g_fd file);

private:
	loader_status_t load_image(elf32_ehdr* hdr);
	loader_status_t load_tls_segment(elf32_phdr* phdr);
	loader_status_t load_load_segment(elf32_phdr* phdr);

	static elf32_validation_status_t validate(elf32_ehdr* header);

};

#endif
