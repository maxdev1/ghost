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

#ifndef __LOADER__
#define __LOADER__

#include <ghost.h>
#include <string>

// maximum number of pages to allocate & load at once
#define MAXIMUM_LOAD_PAGES_AT_ONCE		0x10

// loader status codes
enum loader_status_t {
	LS_SUCCESSFUL, LS_FORMAT_ERROR, LS_IO_ERROR, LS_MEMORY_ERROR, LS_UNKNOWN
};

/**
 *
 */
class loader_t {
protected:
	g_process_creation_identifier proc_ident;
	g_fd file;

public:
	loader_t(g_process_creation_identifier target, g_fd file) :
			proc_ident(target), file(file) {
	}
	virtual ~loader_t() {
	}

	virtual loader_status_t load(uintptr_t* out_entry_addr) = 0;
};

#endif
