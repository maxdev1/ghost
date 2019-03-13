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

#ifndef GHOST_SHARED_FILESYSTEM_FSSTD
#define GHOST_SHARED_FILESYSTEM_FSSTD

#include "ghost/common.h"
#include "ghost/stdint.h"
#include "ghost/types.h"

__BEGIN_C

/**
 * Types
 */
typedef int32_t g_fd; // a file descriptor
typedef uint32_t g_fs_virt_id; // a vfs node id
typedef uint64_t g_fs_phys_id; // a physical filesystem node identifier

#define G_FD_NONE		((g_fd) -1)

/**
 * Limit constants
 */
#define G_PATH_MAX		4096
#define G_FILENAME_MAX	512

/**
 * File mode flags
 */
typedef uint32_t g_file_flag_mode;
#define G_FILE_FLAGS_MODE_RANGE				(0xFFFF)	// flags are in this range
#define G_FILE_FLAG_MODE_READ				((g_file_flag_mode) (1 << 0))	// read mode
#define G_FILE_FLAG_MODE_WRITE				((g_file_flag_mode) (1 << 1))	// write mode
#define G_FILE_FLAG_MODE_BINARY				((g_file_flag_mode) (1 << 2))	// binary mode
#define G_FILE_FLAG_MODE_TEXTUAL			((g_file_flag_mode) (1 << 3))	// textual mode
#define G_FILE_FLAG_MODE_TRUNCATE			((g_file_flag_mode) (1 << 4))	// truncate mode
#define G_FILE_FLAG_MODE_APPEND				((g_file_flag_mode) (1 << 5))	// append mode
#define G_FILE_FLAG_MODE_CREATE				((g_file_flag_mode) (1 << 6))	// create mode
#define G_FILE_FLAG_MODE_EXCLUSIVE			((g_file_flag_mode) (1 << 7))	// exclusive mode
#define G_FILE_FLAG_MODE_BLOCKING			((g_file_flag_mode) (1 << 8))	// blocking mode
// currently, 15 bits are reserved for flags. adjust mode mask if necessary

/**
 * Seek modes
 */
typedef int32_t g_fs_seek_mode;
#define G_FS_SEEK_SET ((g_fs_seek_mode) 0) /* set absolute offset */
#define G_FS_SEEK_CUR ((g_fs_seek_mode) 1) /* set to current offset plus amount */
#define G_FS_SEEK_END ((g_fs_seek_mode) 2) /* set offset to EOF plus offset */

/**
 * Types of filesystem nodes
 */
typedef int g_fs_node_type;
#define G_FS_NODE_TYPE_NONE ((g_fs_node_type) 0)
#define G_FS_NODE_TYPE_ROOT ((g_fs_node_type) 1)
#define G_FS_NODE_TYPE_MOUNTPOINT ((g_fs_node_type) 2)
#define G_FS_NODE_TYPE_FOLDER ((g_fs_node_type) 3)
#define G_FS_NODE_TYPE_FILE ((g_fs_node_type) 4)
#define G_FS_NODE_TYPE_PIPE ((g_fs_node_type) 5)

/**
 * Stat attributes
 */
typedef struct {
	uint32_t mode;
}__attribute__((packed)) g_fs_stat_attributes;

/**
 * Create delegate status
 */
typedef int32_t g_fs_register_as_delegate_status;
#define G_FS_REGISTER_AS_DELEGATE_SUCCESSFUL ((g_fs_register_as_delegate_status) 0)
#define G_FS_REGISTER_AS_DELEGATE_FAILED_EXISTING ((g_fs_register_as_delegate_status) 1)
#define G_FS_REGISTER_AS_DELEGATE_FAILED_DELEGATE_CREATION ((g_fs_register_as_delegate_status) 2)

/**
 * Transaction IDs
 */
typedef uint64_t g_fs_transaction_id;
#define G_FS_TRANSACTION_NO_REPEAT_ID ((g_fs_transaction_id) -1)

/**
 * Status codes for transactions
 */
typedef int g_fs_transaction_status;
#define G_FS_TRANSACTION_WAITING ((g_fs_transaction_status) 0) // transaction is waiting for the delegate
#define G_FS_TRANSACTION_FINISHED ((g_fs_transaction_status) 1) // transaction is finished
#define G_FS_TRANSACTION_REPEAT ((g_fs_transaction_status) 2) // transaction must call handler again

/**
 * Status codes for the {g_fs_create_node} system call
 */
typedef int g_fs_create_node_status;
#define G_FS_CREATE_NODE_STATUS_CREATED ((g_fs_create_node_status) 0)
#define G_FS_CREATE_NODE_STATUS_UPDATED ((g_fs_create_node_status) 1)
#define G_FS_CREATE_NODE_STATUS_FAILED_NO_PARENT ((g_fs_create_node_status) 2)

/**
 * Status codes for internal use during discovery
 */
typedef int g_fs_discovery_status;
#define G_FS_DISCOVERY_SUCCESSFUL ((g_fs_discovery_status) 0)
#define G_FS_DISCOVERY_NOT_FOUND ((g_fs_discovery_status) 1)
#define G_FS_DISCOVERY_BUSY ((g_fs_discovery_status) 2)
#define G_FS_DISCOVERY_ERROR ((g_fs_discovery_status) 3)

/**
 * Types of request messages that the kernel might send to a tasked fs delegate
 */
typedef int g_fs_tasked_delegate_request_type;
#define G_FS_TASKED_DELEGATE_REQUEST_TYPE_DISCOVER ((g_fs_tasked_delegate_request_type) 0)
#define G_FS_TASKED_DELEGATE_REQUEST_TYPE_READ ((g_fs_tasked_delegate_request_type) 1)
#define G_FS_TASKED_DELEGATE_REQUEST_TYPE_WRITE ((g_fs_tasked_delegate_request_type) 2)
#define G_FS_TASKED_DELEGATE_REQUEST_TYPE_GET_LENGTH ((g_fs_tasked_delegate_request_type) 3)
#define G_FS_TASKED_DELEGATE_REQUEST_TYPE_READ_DIRECTORY ((g_fs_tasked_delegate_request_type) 4)
#define G_FS_TASKED_DELEGATE_REQUEST_TYPE_OPEN ((g_fs_tasked_delegate_request_type) 5)
#define G_FS_TASKED_DELEGATE_REQUEST_TYPE_CLOSE ((g_fs_tasked_delegate_request_type) 6)

/**
 * Status codes for the {g_fs_open} system call
 */
typedef int g_fs_open_status;
#define G_FS_OPEN_SUCCESSFUL ((g_fs_open_status) 0)
#define G_FS_OPEN_NOT_FOUND ((g_fs_open_status) 1)
#define G_FS_OPEN_ERROR ((g_fs_open_status) 2)
#define G_FS_OPEN_BUSY ((g_fs_open_status) 3)

/**
 * Status codes for the {g_fs_read} system call
 */
typedef int g_fs_read_status;
#define G_FS_READ_SUCCESSFUL ((g_fs_read_status) 0)
#define G_FS_READ_INVALID_FD ((g_fs_read_status) 1)
#define G_FS_READ_BUSY ((g_fs_read_status) 2)
#define G_FS_READ_ERROR ((g_fs_read_status) 3)

/**
 * Status codes for the {g_fs_write} system call
 */
typedef int g_fs_write_status;
#define G_FS_WRITE_SUCCESSFUL ((g_fs_write_status) 0)
#define G_FS_WRITE_INVALID_FD ((g_fs_write_status) 1)
#define G_FS_WRITE_NOT_SUPPORTED ((g_fs_write_status) 2)
#define G_FS_WRITE_BUSY ((g_fs_write_status) 3)
#define G_FS_WRITE_ERROR ((g_fs_write_status) 4)

/**
 * Status codes for the {g_fs_close} system call
 */
typedef int g_fs_close_status;
#define G_FS_CLOSE_SUCCESSFUL ((g_fs_close_status) 0)
#define G_FS_CLOSE_INVALID_FD ((g_fs_close_status) 1)
#define G_FS_CLOSE_BUSY ((g_fs_close_status) 2)
#define G_FS_CLOSE_ERROR ((g_fs_close_status) 3)

/**
 * Status codes for the {g_fs_seek} system call
 */
typedef int g_fs_seek_status;
#define G_FS_SEEK_SUCCESSFUL ((g_fs_seek_status) 0)
#define G_FS_SEEK_INVALID_FD ((g_fs_seek_status) 1)
#define G_FS_SEEK_ERROR ((g_fs_seek_status) 2)

/**
 * Status codes for the {g_fs_tell} system call
 */
typedef int g_fs_tell_status;
#define G_FS_TELL_SUCCESSFUL ((g_fs_tell_status) 0)
#define G_FS_TELL_INVALID_FD ((g_fs_tell_status) 1)

/**
 * Status codes for the {g_fs_length} system call
 */
typedef int g_fs_length_status;
#define G_FS_LENGTH_SUCCESSFUL ((g_fs_length_status) 0)
#define G_FS_LENGTH_INVALID_FD ((g_fs_length_status) 1)
#define G_FS_LENGTH_NOT_FOUND ((g_fs_length_status) 2)
#define G_FS_LENGTH_BUSY ((g_fs_length_status) 3)
#define G_FS_LENGTH_ERROR ((g_fs_length_status) 4)

/**
 * Status codes for the {g_fs_clonefd} system call
 */
typedef int g_fs_clonefd_status;
#define G_FS_CLONEFD_SUCCESSFUL ((g_fs_clonefd_status) 0)
#define G_FS_CLONEFD_INVALID_SOURCE_FD ((g_fs_clonefd_status) 1)
#define G_FS_CLONEFD_ERROR ((g_fs_clonefd_status) 2)

/**
 * Status codes for the {g_fs_pipe} system call
 */
typedef int g_fs_pipe_status;
#define G_FS_PIPE_SUCCESSFUL ((g_fs_pipe_status) 0)
#define G_FS_PIPE_ERROR ((g_fs_pipe_status) 1)

/**
 * Status codes for the {g_set_working_directory} system call
 */
typedef int g_set_working_directory_status;
#define G_SET_WORKING_DIRECTORY_SUCCESSFUL ((g_set_working_directory_status) 0)
#define G_SET_WORKING_DIRECTORY_NOT_A_FOLDER ((g_set_working_directory_status) 1)
#define G_SET_WORKING_DIRECTORY_NOT_FOUND ((g_set_working_directory_status) 2)
#define G_SET_WORKING_DIRECTORY_ERROR ((g_set_working_directory_status) 3)

/**
 * Status codes for the {g_get_working_directory} system call
 */
typedef int g_get_working_directory_status;
#define G_GET_WORKING_DIRECTORY_SUCCESSFUL ((g_get_working_directory_status) 0)
#define G_GET_WORKING_DIRECTORY_SIZE_EXCEEDED ((g_get_working_directory_status) 1)
#define G_GET_WORKING_DIRECTORY_ERROR ((g_get_working_directory_status) 2)

/**
 * Status codes & structures for directory reading
 */
typedef int g_fs_open_directory_status;
#define G_FS_OPEN_DIRECTORY_SUCCESSFUL ((g_fs_open_directory_status) 0)
#define G_FS_OPEN_DIRECTORY_NOT_FOUND ((g_fs_open_directory_status) 1)
#define G_FS_OPEN_DIRECTORY_ERROR ((g_fs_open_directory_status) 2)

typedef int g_fs_read_directory_status;
#define G_FS_READ_DIRECTORY_SUCCESSFUL ((g_fs_read_directory_status) 0)
#define G_FS_READ_DIRECTORY_EOD ((g_fs_read_directory_status) 1)
#define G_FS_READ_DIRECTORY_ERROR ((g_fs_read_directory_status) 2)

typedef int g_fs_directory_refresh_status;
#define G_FS_DIRECTORY_REFRESH_SUCCESSFUL ((g_fs_directory_refresh_status) 0)
#define G_FS_DIRECTORY_REFRESH_ERROR ((g_fs_directory_refresh_status) 1)
#define G_FS_DIRECTORY_REFRESH_BUSY ((g_fs_directory_refresh_status) 2)

typedef struct {
	g_fs_virt_id node_id;
	g_fs_node_type type;
	char* name;
} g_fs_directory_entry;

typedef struct {
	g_fs_virt_id node_id;
	int position;
	g_fs_directory_entry entry_buffer;
} g_fs_directory_iterator;

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
