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

#ifndef GHOST_API_KERNQUERY
#define GHOST_API_KERNQUERY

#include "common.h"
#include "kernquery/types.h"

__BEGIN_C

/**
 * Queries kernel task, process or system resource information.
 *
 * The command selects the structure layout; this call has no buffer-size argument. Task lists may
 * be truncated to id_buffer_size; filled_ids reports the number written.
 *
 * @param[in] command One of the G_KERNQUERY_* query commands.
 * @param[in,out] buffer Non-null command-specific g_kernquery_*_data structure. Initialize query
 *                       inputs and any nested buffer pointers and capacities before calling.
 * @return A g_kernquery_status code; output fields are valid only as specified for that status.
 * @security-level APPLICATION
 */
g_kernquery_status g_kernquery(uint16_t command, uint8_t* buffer);

__END_C

#endif
