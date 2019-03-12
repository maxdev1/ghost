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

#ifndef __KERNEL_IPC_PIPES__
#define __KERNEL_IPC_PIPES__

#include "ghost.h"

/**
 * Creates a new pipe.
 */
g_fs_phys_id pipeCreate();

g_fs_read_status pipeRead(g_fs_phys_id pipeId, uint8_t* buffer, uint64_t offset, uint64_t length, int64_t* outRead);

g_fs_write_status pipeWrite(g_fs_phys_id pipeId, uint8_t* buffer, uint64_t offset, uint64_t length, int64_t* outWrote);

g_fs_length_status pipeGetLength(g_fs_phys_id pipeId, uint64_t* outLength);

g_fs_open_status pipeTruncate(g_fs_phys_id pipeId);

#endif
