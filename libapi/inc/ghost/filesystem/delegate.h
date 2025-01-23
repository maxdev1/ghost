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

#ifndef GHOST_API_FILESYSTEM_DELEGATE
#define GHOST_API_FILESYSTEM_DELEGATE

#include "types.h"
#include "../memory/types.h"

__BEGIN_C

/**
 * Transaction storage structures (NOTE limited to 1 page!)
 */
typedef struct {
	g_fs_phys_id parent_phys_fs_id;
	char name[G_FILENAME_MAX];

	g_fs_discovery_status result_status;
} g_fs_tasked_delegate_transaction_storage_discovery;

typedef struct {
	g_fs_phys_id phys_fs_id;
	void* mapped_buffer;
	int64_t offset;
	int64_t length;
	g_virtual_address mapping_start;
	int32_t mapping_pages;

	int64_t result_read;
	g_fs_read_status result_status;
} g_fs_tasked_delegate_transaction_storage_read;

typedef struct {
	g_fs_phys_id phys_fs_id;
	void* mapped_buffer;
	int64_t offset;
	int64_t length;
	g_virtual_address mapping_start;
	int32_t mapping_pages;

	int64_t result_write;
	g_fs_read_status result_status;
} g_fs_tasked_delegate_transaction_storage_write;

typedef struct {
	g_fs_phys_id phys_fs_id;

	int64_t result_length;
	g_fs_read_status result_status;
} g_fs_tasked_delegate_transaction_storage_get_length;

typedef struct {
	g_fs_phys_id parent_phys_fs_id;
	g_fs_phys_id parent_virt_fs_id;

	g_fs_directory_refresh_status result_status;
} g_fs_tasked_delegate_transaction_storage_directory_refresh;

typedef struct {
	g_fs_phys_id phys_fs_id;
	char name[G_FILENAME_MAX];

	g_fs_open_status result_status;
} g_fs_tasked_delegate_transaction_storage_open;

typedef struct {
	g_fs_phys_id phys_fs_id;

	g_fs_close_status result_status;
} g_fs_tasked_delegate_transaction_storage_close;

__END_C

#endif
