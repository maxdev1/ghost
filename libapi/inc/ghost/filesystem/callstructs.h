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

#ifndef GHOST_API_FILESYSTEM_CALLS
#define GHOST_API_FILESYSTEM_CALLS

#include "types.h"
#include "delegate.h"
#include "../tasks/types.h"
#include "../memory/types.h"

/**
 * @field path
 * 		buffer containing the file path
 *
 * @field flags
 * 		open flags
 *
 * @field status
 * 		one of the {g_fs_open_status} codes
 *
 * @field fd
 * 		resulting file descriptor
 *
 * @security-level APPLICATION
 */
typedef struct
{
    char* path;
    g_file_flag_mode flags;

    g_fs_open_status status;
    g_fd fd;
}__attribute__((packed)) g_syscall_fs_open;

/**
 * @field fd
 * 		file descriptor
 *
 * @field buffer
 * 		output buffer
 *
 * @field length
 * 		number of bytes to read
 *
 * @field status
 * 		one of the {g_fs_read_status} codes
 *
 * @field result
 * 		number of bytes read
 *
 * @security-level APPLICATION
 */
typedef struct
{
    g_fd fd;
    uint8_t* buffer;
    int64_t length;

    g_fs_read_status status;
    int64_t result;
}__attribute__((packed)) g_syscall_fs_read;

/**
 * @field fd
 * 		file descriptor
 *
 * @field buffer
 * 		output buffer
 *
 * @field length
 * 		number of bytes to write
 *
 * @field status
 * 		one of the {g_fs_write_status} codes
 *
 * @field result
 * 		number of bytes write
 *
 * @security-level APPLICATION
 */
typedef struct
{
    g_fd fd;
    uint8_t* buffer;
    int64_t length;

    g_fs_write_status status;
    int64_t result;
}__attribute__((packed)) g_syscall_fs_write;

/**
 * @field fd
 * 		file descriptor
 *
 * @field status
 * 		one of the {g_fs_close_status} codes
 *
 * @security-level APPLICATION
 */
typedef struct
{
    g_fd fd;

    g_fs_close_status status;
}__attribute__((packed)) g_syscall_fs_close;

/**
 * @field path
 * 		file path
 *
 * @field follow_symlinks
 * 		whether to follow symbolic links
 *
 * @field out
 * 		stat output struct
 *
 * @field result
 * 		the call result
 *
 * @security-level APPLICATION
 */
typedef struct
{
    const char* path;
    g_bool follow_symlinks;
    g_fs_stat_data* out;

    g_fs_stat_status status;
}__attribute__((packed)) g_syscall_fs_stat;

/**
 * @field fd
* 		file descriptor
 *
 * @field out
 * 		stat output struct
 *
 * @field result
 * 		the call result
 *
 * @security-level APPLICATION
 */
typedef struct
{
    g_fd fd;
    g_fs_stat_data* out;

    g_fs_stat_status status;
}__attribute__((packed)) g_syscall_fs_fstat;

/**
 * @field source_fd
 * 		source file descriptor
 *
 * @field source_pid
 * 		source process id
 *
 * @field target_fd
 * 		target file descriptor
 *
 * @field target_pid
 * 		target process id
 *
 * @field status
 * 		status of the operation
 *
 * @field result
 * 		the resulting file descriptor
 *
 * @security-level APPLICATION
 */
typedef struct
{
    g_fd source_fd;
    g_pid source_pid;
    g_fd target_fd;
    g_pid target_pid;

    g_fs_clonefd_status status;
    g_fd result;
}__attribute__((packed)) g_syscall_fs_clonefd;

/**
 * @field write_fd
 * 		write end file descriptor of created pipe
 *
 * @field read_fd
 * 		read end file descriptor of created pipe
 *
 * @field status
 * 		the call status
 *
 * @security-level APPLICATION
 */
typedef struct
{
    g_fd write_fd;
    g_fd read_fd;
    g_fs_pipe_status status;
    g_bool blocking;
}__attribute__((packed)) g_syscall_fs_pipe;

/**
 * @field mode
 * 		the mode flags
 *
 * @field path
 * 		buffer containing the file path
 *
 * @field fd
 * 		contains the fd
 *
 * @field status
 * 		the status code
 *
 * @field length
 * 		length in bytes
 *
 * @security-level APPLICATION
 */
typedef struct
{
    g_fd fd;

    g_fs_length_status status;
    int64_t length;
}__attribute__((packed)) g_syscall_fs_length;

/**
 * @field fd
 * 		file descriptor
 *
 * @field amount
 * 		amount to seek
 *
 * @field mode
 * 		mode to seek with
 *
 * @field status
 * 		one of the {g_fs_seek_status} codes
 *
 * @field result
 * 		resulting offset
 *
 * @security-level APPLICATION
 */
typedef struct
{
    g_fd fd;
    int64_t amount;
    g_fs_seek_mode mode;

    g_fs_seek_status status;
    int64_t result;
}__attribute__((packed)) g_syscall_fs_seek;

/**
 * @field fd
 * 		file descriptor
 *
 * @field status
 * 		status code
 *
 * @field result
 * 		current offset
 *
 * @security-level APPLICATION
 */
typedef struct
{
    g_fd fd;

    g_fs_tell_status status;
    int64_t result;
}__attribute__((packed)) g_syscall_fs_tell;

/**
 * Registers the calling task as a delegate. version and storage_size negotiate
 * the shared-storage ABI before the mount becomes visible.
 *
 * @security-level DRIVER
 */
typedef struct
{
	const char* name;
	g_fs_phys_id phys_mountpoint_id;
	uint32_t version;
	uint32_t storage_size;

	g_fs_virt_id mountpoint_id;
	g_fs_delegate_transaction_storage* transaction_storage;
	g_fs_delegate_status result;
} __attribute__((packed)) g_syscall_fs_delegate_register;

/**
 * Completes an owned transaction using its shared operation result. The kernel
 * validates results against its private request snapshot, including on cancellation.
 *
 * @security-level DRIVER
 */
typedef struct
{
	g_fs_transaction_id transaction;

	g_fs_delegate_status result;
} __attribute__((packed)) g_syscall_fs_delegate_transaction_complete;

/**
 * Publishes a node for an active lookup or directory-refresh transaction.
 * The parent must belong to that transaction's delegate mount.
 * @security-level DRIVER
 */
typedef struct
{
	g_fs_transaction_id transaction;
	g_fs_virt_id parent_id;
	const char* name;
	g_fs_node_type type;
	g_fs_phys_id phys_fs_id;

	g_fs_virt_id created_id;
	g_fs_delegate_status result;
} __attribute__((packed)) g_syscall_fs_delegate_node_create;

/**
 * Unregisters an owned mount between transactions. An active transaction must
 * first be completed or acknowledged as cancelled.
 * @security-level DRIVER
 */
typedef struct
{
	g_fs_virt_id mountpoint_id;

	g_fs_delegate_status result;
} __attribute__((packed)) g_syscall_fs_delegate_unregister;

/**
 * @field path
 * 		buffer containing the folders path
 *
 * @field iterator
 * 		pointer to an allocated iterator
 *
 * @field status
 * 		one of the {g_fs_open_directory_status} codes
 *
 * @security-level APPLICATION
 */
typedef struct
{
    char* path;
    g_fs_directory_iterator* iterator;

    g_fs_open_directory_status status;
}__attribute__((packed)) g_syscall_fs_open_directory;

/**
 * @field iterator
 * 		pointer to the iterator
 *
 * @field status
 * 		one of the {g_fs_directory_refresh_status} codes
 *
 * @security-level APPLICATION
 */
typedef struct
{
    g_fs_directory_iterator* iterator;

    g_fs_directory_refresh_status status;
}__attribute__((packed)) g_syscall_fs_read_directory;

/**
 * @field iterator
 * 		pointer to the iterator
 *
 * @security-level APPLICATION
 */
typedef struct
{
    g_fs_directory_iterator* iterator;
}__attribute__((packed)) g_syscall_fs_close_directory;

/**
 * @field in
 * 		source path buffer
 * @field out
 * 		result buffer
 * @field status
 * 		status of the command
 */
typedef struct
{
    const char* in;
    char* out;

    g_fs_real_path_status status;
} __attribute__((packed)) g_syscall_fs_real_path;

typedef struct
{
	const char* path;
	g_fs_mutation_status status;
} __attribute__((packed)) g_syscall_fs_path_mutation;

typedef struct
{
	g_fd fd;
	uint64_t length;
	g_fs_mutation_status status;
} __attribute__((packed)) g_syscall_fs_fd_mutation;

#endif
