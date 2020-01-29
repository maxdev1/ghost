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

#ifndef __GDT_MACROS__
#define __GDT_MACROS__

/**
 * Macro to create the ACCESS byte for a GDT entry to a code or data segment
 *
 * Parameters:
 *   readWrite       if code read allowed, if data write allowed
 *   codeOrData      determines if its a code (1) or a data (0) segment
 *   privilege       Privilege level (Ring 0 - Ring 3)
 *   present         1 if present
 *
 * Bits:           Values:                    Info:
 *   0               0                          Accessed bit, free for developers use
 *   1               parameter readWrite        read if code segment, or write if data segment
 *   2               0                          Direction bit or conforming bit)
 *   3               parameter codeOrData       Entry is a code (1) or data (0) segment
 *   4               0                          Entry is for Code/Data (1) or Gate/TSS (0)
 *   5 - 6           parameter privilege        Ring 0 - Ring 3
 *   7               parameter present          Must be 1 for active entry
 */
#define G_ACCESS_BYTE__FOR__CODE_OR_DATA(readWrite, codeOrData, privilege, present) \
	(0 | (readWrite << 1) | (0 << 2) | (codeOrData << 3) | (1 << 4) | (privilege << 5) | (present << 7))

/**
 * Macro to create the ACCESS byte for a GDT entry to a Gate/TSS
 *
 * Parameters:
 *   segmentType     e.g. 0x9 for 386-TSS
 *   privilege       Privilege level (Ring 0 - Ring 3)
 *   present         1 if present
 *
 * Bits:           Values:                    Info:
 *   0 - 3           parameter segmentType      Segment type
 *   4               0                          Entry is for Code/Data (1) or Gate/TSS (0)
 *   5 - 6           parameter privilege        Ring 0 - Ring 3
 *   7               parameter present          Must be 1 for active entry
 */
#define G_ACCESS_BYTE__FOR__GATE_OR_TSS(segmentType, privilege, present) \
	(segmentType | (0 << 4) | (privilege << 5) | (present << 7))

/**
 * Common descriptor access flags
 */
#define G_ACCESS_BYTE__KERNEL_CODE_SEGMENT G_ACCESS_BYTE__FOR__CODE_OR_DATA(1, 1, 0, 1)
#define G_ACCESS_BYTE__KERNEL_DATA_SEGMENT G_ACCESS_BYTE__FOR__CODE_OR_DATA(1, 0, 0, 1)
#define G_ACCESS_BYTE__USER_CODE_SEGMENT G_ACCESS_BYTE__FOR__CODE_OR_DATA(1, 1, 3, 1)
#define G_ACCESS_BYTE__USER_DATA_SEGMENT G_ACCESS_BYTE__FOR__CODE_OR_DATA(1, 0, 3, 1)
#define G_ACCESS_BYTE__TSS_386_SEGMENT G_ACCESS_BYTE__FOR__GATE_OR_TSS(0x9, 0, 1)

// Descriptor index constants
#define G_GDT_DESCRIPTOR_KERNEL_CODE 0x08
#define G_GDT_DESCRIPTOR_KERNEL_DATA 0x10
#define G_GDT_DESCRIPTOR_USER_CODE 0x18
#define G_GDT_DESCRIPTOR_USER_DATA 0x20
#define G_GDT_DESCRIPTOR_TSS 0x28
#define G_GDT_DESCRIPTOR_USERTHREADPTR 0x30

#define G_SEGMENT_SELECTOR_RING0 0 // 00
#define G_SEGMENT_SELECTOR_RING3 3 // 11

#endif
