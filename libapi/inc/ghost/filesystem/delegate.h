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

#define G_FS_DELEGATE_VERSION ((uint32_t) 3)
#define G_FS_DELEGATE_MESSAGE_SENDER ((int32_t) -1)

/**
 * Kernel notification payload and shared-storage header. The notification is
 * received through g_receive_message with sender G_FS_DELEGATE_MESSAGE_SENDER;
 * reject other senders. Its mount/transaction must match the corresponding storage
 * page. storage_size covers the entire storage structure, including this header.
 * Header fields and request inputs are kernel-owned; the delegate writes only
 * the selected operation's result fields.
 */
typedef struct
{
	uint32_t version;
	uint32_t storage_size;
	g_fs_transaction_id transaction;
	g_fs_delegate_request_type type;
	g_fs_virt_id mountpoint_id;
} g_fs_delegate_request;

typedef struct
{
	g_fs_phys_id parent_phys_fs_id;
	g_fs_virt_id parent_virt_fs_id;
	char name[G_FILENAME_MAX];

	g_fs_virt_id result_node_id;
	g_fs_lookup_status result_status;
} g_fs_delegate_lookup;

/**
 * mapped_buffer is a writable mapping in the delegate's address space. The
 * kernel owns its mapping and backing references; it remains valid until the
 * transaction is acknowledged as completed or cancelled. A timeout alone must
 * not release the mapping while the delegate can still access it. This mapping
 * is not guaranteed to be physically contiguous or suitable for DMA. Before a
 * caller resumes after cancellation, further delegate access must be isolated
 * from memory that the caller can reuse; retaining page references alone is
 * insufficient to prevent late writes to a reused caller buffer.
 * On success, result_read is between zero and length; zero denotes EOF.
 */
typedef struct
{
	g_fs_phys_id phys_fs_id;
	void* mapped_buffer;
	uint64_t offset;
	uint64_t length;

	int64_t result_read;
	g_fs_read_status result_status;
} g_fs_delegate_read;

/**
 * The write mapping follows the same lifetime as a read mapping, but its bytes
 * must be treated as read-only by the delegate. On success, result_write is
 * between zero and length. Mapping bounds and cleanup state remain kernel-private.
 */
typedef struct
{
	g_fs_phys_id phys_fs_id;
	const void* mapped_buffer;
	uint64_t offset;
	uint64_t length;

	int64_t result_write;
	g_fs_write_status result_status;
} g_fs_delegate_write;

typedef struct
{
	g_fs_phys_id phys_fs_id;

	uint64_t result_length;
	g_fs_length_status result_status;
} g_fs_delegate_get_length;

typedef struct
{
	g_fs_phys_id parent_phys_fs_id;
	g_fs_virt_id parent_virt_fs_id;

	g_fs_directory_refresh_status result_status;
} g_fs_delegate_directory_refresh;

typedef struct
{
	g_fs_phys_id phys_fs_id;
	g_file_flag_mode flags;
	char name[G_FILENAME_MAX];

	g_fs_open_status result_status;
} g_fs_delegate_open;

typedef struct
{
	g_fs_phys_id phys_fs_id;
	g_file_flag_mode flags;

	g_fs_close_status result_status;
} g_fs_delegate_close;

typedef struct
{
	g_fs_phys_id parent_phys_fs_id;
	g_fs_virt_id parent_virt_fs_id;
	g_fs_node_type type;
	char name[G_FILENAME_MAX];
	g_fs_virt_id result_node_id;
	g_fs_mutation_status result_status;
} g_fs_delegate_create;

typedef struct
{
	g_fs_phys_id phys_fs_id;
	uint64_t length;
	g_fs_mutation_status result_status;
} g_fs_delegate_resize;

typedef struct
{
	g_fs_phys_id phys_fs_id;
	g_fs_node_type type;
	g_fs_mutation_status result_status;
} g_fs_delegate_remove;

typedef struct
{
	g_fs_phys_id phys_fs_id;
	g_fs_mutation_status result_status;
} g_fs_delegate_flush;

typedef struct
{
	g_fs_phys_id phys_fs_id;
	g_fs_phys_id parent_phys_fs_id;
	g_fs_phys_id replaced_phys_fs_id;
	char name[G_FILENAME_MAX];
	g_fs_mutation_status result_status;
} g_fs_delegate_rename;

/**
 * One shared control page per registered delegate, with one active transaction
 * per page. The kernel publishes the header and selected payload before sending
 * a notification. The delegate publishes its results through transaction_complete;
 * cancellation does not permit page reuse until the delegate acknowledges it or
 * its serving task has stopped. The kernel keeps authoritative transaction IDs,
 * inputs, mapping ownership and bounds outside this writable page.
 */
typedef struct
{
	g_fs_delegate_request header;
	union
	{
		g_fs_delegate_lookup lookup;
		g_fs_delegate_read read;
		g_fs_delegate_write write;
		g_fs_delegate_get_length get_length;
		g_fs_delegate_directory_refresh directory_refresh;
		g_fs_delegate_open open;
		g_fs_delegate_close close;
		g_fs_delegate_create create;
		g_fs_delegate_resize resize;
		g_fs_delegate_remove remove;
		g_fs_delegate_flush flush;
		g_fs_delegate_rename rename;
	} data;
} g_fs_delegate_transaction_storage;

__END_C

#endif
