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

#ifndef GHOST_API_CALLS_FILESYSTEMCALLS
#define GHOST_API_CALLS_FILESYSTEMCALLS

#include "ghost/fs.h"

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
typedef struct {
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
typedef struct {
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
typedef struct {
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
typedef struct {
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
 * @field result
 * 		the call result
 *
 * @security-level APPLICATION
 */
typedef struct {
	char* path;
	uint8_t follow_symlinks;

	g_fs_stat_attributes stats;
	int32_t result;
}__attribute__((packed)) g_syscall_fs_stat;

/**
 * @field fd
 * 		file descriptor
 *
 * @field follow_symlinks
 * 		whether to follow symbolic links
 *
 * @field result
 * 		the call result
 *
 * @security-level APPLICATION
 */
typedef struct {
	g_fd fd;

	g_fs_stat_attributes stats;
	int32_t result;
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
typedef struct {
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
typedef struct {
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
typedef int g_syscall_fs_length_mode;
#define		G_SYSCALL_FS_LENGTH_MODE_BY_MASK			1
#define	 	G_SYSCALL_FS_LENGTH_BY_PATH					((g_syscall_fs_length_mode) 0x0)
#define 	G_SYSCALL_FS_LENGTH_BY_FD					((g_syscall_fs_length_mode) 0x1)
#define 	G_SYSCALL_FS_LENGTH_MODE_SYMLINK_MASK		2
#define 	G_SYSCALL_FS_LENGTH_FOLLOW_SYMLINKS			((g_syscall_fs_length_mode) 0x0)
#define 	G_SYSCALL_FS_LENGTH_NOT_FOLLOW_SYMLINKS		((g_syscall_fs_length_mode) 0x2)

typedef struct {
	g_syscall_fs_length_mode mode;
	char* path;
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
typedef struct {
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
typedef struct {
	g_fd fd;

	g_fs_tell_status status;
	int64_t result;
}__attribute__((packed)) g_syscall_fs_tell;

/**
 * @field path
 * 		buffer containing the path
 *
 * @field result
 * 		one of the {g_set_working_directory_status} codes
 *
 * @security-level APPLICATION
 */
typedef struct {
	char* path;

	g_set_working_directory_status result;
}__attribute__((packed)) g_syscall_fs_set_working_directory;

/**
 * @field buffer
 * 		buffer with the given size
 *
 * @field maxlen
 * 		maximum number of bytes to write to the buffer
 *
 * @security-level APPLICATION
 */
typedef struct {
	char* buffer;
	size_t maxlen;
	g_get_working_directory_status result;
}__attribute__((packed)) g_syscall_fs_get_working_directory;

/**
 * @field buffer
 * 		buffer with a size of at least {G_PATH_MAX} bytes
 *
 * @security-level APPLICATION
 */
typedef struct {
	char* buffer;
}__attribute__((packed)) g_syscall_fs_get_executable_path;

/**
 * @field name
 * 		the name to use for delegate registration
 *
 * @field result
 * 		one of the {G_FS_REGISTER_AS_DELEGATE_*} status codes
 *
 * @field phys_mountpoint_id
 * 		the physical id to set for the mountpoint
 *
 * @field mountpoint_id
 * 		contains the mountpoint id on success
 *
 * @field transaction_storage
 * 		contains the transaction storage address on success
 *
 * @security-level DRIVER
 */
typedef struct {
	char* name;
	g_fs_phys_id phys_mountpoint_id;

	g_fs_virt_id mountpoint_id;
	g_address transaction_storage;
	g_fs_register_as_delegate_status result;
}__attribute__((packed)) g_syscall_fs_register_as_delegate;

/**
 * @field transaction
 * 		the transaction to set the status for
 *
 * @field status
 * 		the status to set
 *
 * @security-level DRIVER
 */
typedef struct {
	g_fs_transaction_id transaction;
	g_fs_transaction_status status;
}__attribute__((packed)) g_syscall_fs_set_transaction_status;

/**
 * @field parent_id
 * 		id of the parent node
 *
 * @field name
 *		the name to set
 *
 * @field type
 * 		the type to set
 *
 * @field phys_fs_id
 * 		physical fs unique id for the node
 *
 * @field create_id
 * 		is filled with the id of the created node
 *
 * @security-level DRIVER
 */
typedef struct {
	g_fs_virt_id parent_id;
	char* name;
	g_fs_node_type type;
	g_fs_phys_id phys_fs_id;

	g_fs_create_node_status result;
	g_fs_virt_id created_id;
}__attribute__((packed)) g_syscall_fs_create_node;

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
typedef struct {
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
typedef struct {
	g_fs_directory_iterator* iterator;

	g_fs_directory_refresh_status status;
}__attribute__((packed)) g_syscall_fs_read_directory;

/**
 * @field iterator
 * 		pointer to the iterator
 *
 * @security-level APPLICATION
 */
typedef struct {
	g_fs_directory_iterator* iterator;
}__attribute__((packed)) g_syscall_fs_close_directory;

#endif
