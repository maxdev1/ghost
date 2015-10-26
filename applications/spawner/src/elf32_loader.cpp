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

#include "elf32_loader.hpp"

#include <string.h>
#include <ghostuser/utils/logger.hpp>
#include <ghostuser/io/files/file_utils.hpp>
#include <stdio.h>

/**
 *
 */
loader_status_t elf32_loader_t::load(uintptr_t* out_entry_addr) {

	// check header
	elf32_ehdr hdr;
	loader_status_t header_stat = read_and_validate_elf_header(file, &hdr);
	if (header_stat != LS_SUCCESSFUL) {
		return header_stat;
	}

	// perform image loading
	loader_status_t status = load_image(&hdr);
	*out_entry_addr = hdr.e_entry;
	return status;
}

/**
 *
 */
loader_status_t elf32_loader_t::read_and_validate_elf_header(g_fd file,
		elf32_ehdr* hdr_buf) {

	// read header
	if (!g_file_utils::read_bytes(file, 0, (uint8_t*) hdr_buf,
			sizeof(elf32_ehdr))) {
		g_logger::log("unable to read ELF header from file %i", file);
		return LS_IO_ERROR;
	}

	// validate header
	elf32_validation_status_t validity = validate(hdr_buf);

	if (validity != ELF32_VALIDATION_SUCCESSFUL) {
		g_logger::log("ELF validation of file %i failed with code %i", file,
				validity);
		return LS_FORMAT_ERROR;
	}

	return LS_SUCCESSFUL;
}

/**
 *
 */
loader_status_t elf32_loader_t::check_for_elf_binary_and_reset(g_fd file) {

	// read and validate ELF header
	elf32_ehdr hdrBuf;
	loader_status_t stat = read_and_validate_elf_header(file, &hdrBuf);

	// reset file
	g_seek(file, 0, G_FS_SEEK_SET);

	return stat;
}

/**
 *
 */
loader_status_t elf32_loader_t::load_image(elf32_ehdr* hdr) {

	// Load segments
	for (uint32_t i = 0; i < hdr->e_phnum; i++) {
		uint32_t phdrOffset = hdr->e_phoff + hdr->e_phentsize * i;
		uint32_t phdrLength = sizeof(elf32_phdr);
		uint8_t phdrBuffer[phdrLength];

		if (!g_file_utils::read_bytes(file, phdrOffset, phdrBuffer,
				phdrLength)) {
			g_logger::log("unable to read segment header from file");
			return LS_IO_ERROR;
		}

		elf32_phdr* phdr = (elf32_phdr*) phdrBuffer;

		if (phdr->p_type == PT_LOAD) {
			loader_status_t seg_stat = load_load_segment(phdr);
			if (seg_stat != LS_SUCCESSFUL) {
				g_logger::log("unable to load PT_LOAD segment from file");
				return seg_stat;
			}

		} else if (phdr->p_type == PT_TLS) {
			loader_status_t seg_stat = load_tls_segment(phdr);
			if (seg_stat != LS_SUCCESSFUL) {
				g_logger::log("unable to load PT_TLS segment from file");
				return seg_stat;
			}
		}
	}

	return LS_SUCCESSFUL;
}

/**
 *
 */
loader_status_t elf32_loader_t::load_tls_segment(elf32_phdr* phdr) {

	// take values
	uint32_t num_bytes_copy = phdr->p_filesz;
	uint32_t num_bytes_zero = phdr->p_memsz;
	uint32_t alignment = phdr->p_align;

	loader_status_t result = LS_UNKNOWN;

	// read contents
	uint8_t* tls_content = new uint8_t[num_bytes_copy];

	if (!g_file_utils::read_bytes(file, phdr->p_offset, (uint8_t*) tls_content,
			num_bytes_copy)) {
		result = LS_IO_ERROR;
		g_logger::log("unable to read TLS segment");

	} else {
		// call kernel to write TLS
		if (g_write_tls_master_for_process(proc_ident, tls_content,
				num_bytes_copy, num_bytes_zero, alignment)) {
			result = LS_SUCCESSFUL;

		} else {
			g_logger::log(
					"unable to allocate TLS storage for spawning process");
			result = LS_MEMORY_ERROR;
		}
	}

	delete tls_content;

	return result;
}

/**
 *
 */
loader_status_t elf32_loader_t::load_load_segment(elf32_phdr* phdr) {

	uint32_t memStart = phdr->p_vaddr & ~0xFFF;
	uint32_t memEnd = ((phdr->p_vaddr + phdr->p_memsz) + 0x1000) & ~0xFFF;

	uint32_t totalPages = (memEnd - memStart) / 0x1000;
	uint32_t loadedPages = 0;

	uint32_t p_offset_in_file = 0;

	while (loadedPages < totalPages) {
		uint32_t step = (totalPages - loadedPages);
		if (step > MAXIMUM_LOAD_PAGES_AT_ONCE) {
			step = MAXIMUM_LOAD_PAGES_AT_ONCE;
		}

		uint32_t startVirt = memStart + loadedPages * 0x1000;
		uint8_t* area = (uint8_t*) g_create_pages_in_spaces(proc_ident,
				startVirt, step);
		if (area == 0) {
			g_logger::log(
					"unable to allocate necessary pages for spawning process");
			return LS_MEMORY_ERROR;
		}

		// Copy content
		uint32_t copyOffsetInArea =
				(startVirt < phdr->p_vaddr) ? (phdr->p_vaddr - startVirt) : 0;
		uint32_t virtualFileEnd = phdr->p_vaddr + phdr->p_filesz;

		// Is there anything left to copy?
		uint32_t copyAmount = 0;
		if (startVirt < virtualFileEnd) {

			// Check if file ends in this area
			if ((virtualFileEnd >= startVirt)
					&& (virtualFileEnd < (startVirt + step * 0x1000))) {
				copyAmount = virtualFileEnd - startVirt - copyOffsetInArea;
			} else {
				copyAmount = step * 0x1000 - copyOffsetInArea;
			}
		}

		// Read file to memory
		if (!g_file_utils::read_bytes(file, phdr->p_offset + p_offset_in_file,
				&area[copyOffsetInArea], copyAmount)) {
			g_logger::log("unable to read LOAD segment");
			return LS_IO_ERROR;
		}

		// Zero area before and after file
		if (copyOffsetInArea > 0) {
			memset(area, 0, copyOffsetInArea);
		}
		if ((copyOffsetInArea + copyAmount) < step * 0x1000) {
			uint32_t endZeroAreaStart = ((uint32_t) area) + copyOffsetInArea
					+ copyAmount;
			uint32_t endZeroAreaLength = (step * 0x1000)
					- (copyOffsetInArea + copyAmount);
			memset((void*) endZeroAreaStart, 0, endZeroAreaLength);
		}

		// Unmap area
		g_unmap(area);
		loadedPages += step;
		p_offset_in_file += copyAmount;
	}

	return LS_SUCCESSFUL;
}

/**
 * Validates the ELF header
 */
elf32_validation_status_t elf32_loader_t::validate(elf32_ehdr* header) {

	// Valid ELF header
	if (/**/(header->e_ident[EI_MAG0] != ELFMAG0) || // 0x7F
			(header->e_ident[EI_MAG1] != ELFMAG1) || // E
			(header->e_ident[EI_MAG2] != ELFMAG2) || // L
			(header->e_ident[EI_MAG3] != ELFMAG3))   // F
			{
		return ELF32_VALIDATION_NOT_ELF;
	}

	// Must be executable
	if (header->e_type != ET_EXEC) {
		return ELF32_VALIDATION_NOT_EXECUTABLE;
	}

	// Must be i386 architecture compatible
	if (header->e_machine != EM_386) {
		return ELF32_VALIDATION_NOT_I386;
	}

	// Must be 32 bit
	if (header->e_ident[EI_CLASS] != ELFCLASS32) {
		return ELF32_VALIDATION_NOT_32BIT;
	}

	// Must be little endian
	if (header->e_ident[EI_DATA] != ELFDATA2LSB) {
		return ELF32_VALIDATION_NOT_LITTLE_ENDIAN;
	}

	// Must comply to current ELF standard
	if (header->e_version != EV_CURRENT) {
		return ELF32_VALIDATION_NOT_STANDARD_ELF;
	}

	// All fine
	return ELF32_VALIDATION_SUCCESSFUL;
}
