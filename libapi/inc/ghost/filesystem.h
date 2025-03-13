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

#ifndef GHOST_API_FILESYSTEM
#define GHOST_API_FILESYSTEM

#include "common.h"
#include "filesystem/types.h"
#include "memory.h"

__BEGIN_C
/**
 * Opens a file.
 *
 * @param path
 * 		the path to the file
 * @param-opt flags
 * 		the flags for open mode
 * @param-opt out_status
 * 		filled with one of the {g_fs_open_status} codes
 *
 * @return the file descriptor for the opened file
 *
 * @security-level APPLICATION
 */
g_fd g_open(const char* path);
g_fd g_open_f(const char* path, g_file_flag_mode flags);
g_fd g_open_fs(const char* path, g_file_flag_mode flags, g_fs_open_status* out_status);

/**
 * Reads bytes from the file to the buffer.
 *
 * @param fd
 * 		the file descriptor
 * @param buffer
 * 		the target buffer
 * @param length
 * 		the length in bytes
 * @param-opt out_status
 * 		filled with one of the {g_fs_read_status} codes
 *
 * @return if the read was successful the length of bytes or
 * 		zero if EOF, otherwise -1
 *
 * @security-level APPLICATION
 */
int32_t g_read(g_fd fd, void* buffer, uint64_t length);
int32_t g_read_s(g_fd fd, void* buffer, uint64_t length, g_fs_read_status* out_status);

/**
 * Writes bytes from the buffer to the file.
 *
 * @param fd
 * 		the file descriptor
 * @param buffer
 * 		the source buffer
 * @param length
 * 		the length in bytes
 * @param-opt out_status
 * 		filled with one of the {g_fs_write_status} codes
 *
 * @return if successful the number of bytes that were written, otherwise -1
 *
 * @security-level APPLICATION
 */
int32_t g_write(g_fd fd, const void* buffer, uint64_t length);
int32_t g_write_s(g_fd fd, const void* buffer, uint64_t length, g_fs_write_status* out_status);

/**
 * Closes a file.
 *
 * @param fd
 * 		the file descriptor to close
 *
 * @return one of the {g_fs_close_status} codes
 *
 * @security-level APPLICATION
 */
g_fs_close_status g_close(g_fd fd);

/**
 * Retrieves the length of a file in bytes.
 *
 * @param fd
 * 		the file descriptor
 *
 * @param out_status
 * 		is filled with the status code
 *
 * @return the length in bytes
 *
 * @security-level APPLICATION
 */
int64_t g_length(g_fd fd);
int64_t g_length_s(g_fd fd, g_fs_length_status* out_status);

/**
 * Opens a directory.
 *
 * @param path
 * 		path of the directory
 *
 * @return pointer to a directory iterator, or 0 if not successful
 */
g_fs_directory_iterator* g_open_directory(const char* path);
g_fs_directory_iterator* g_open_directory_s(const char* path, g_fs_open_directory_status* out_status);

/**
 * Reads the next entry of the directory.
 *
 * @param iterator
 * 		the directory iterator
 *
 * @param out_status
 * 		is filled with the status code
 *
 * @return a directory entry structure, or 0 if not successful
 */
g_fs_directory_entry* g_read_directory(g_fs_directory_iterator* iterator);
g_fs_directory_entry* g_read_directory_s(g_fs_directory_iterator* iterator, g_fs_read_directory_status* out_status);

/**
 * Closes a directory.
 *
 * @param iterator
 * 		the directory iterator
 */
void g_close_directory(g_fs_directory_iterator* iterator);

/**
 * Repositions the offset within a file.
 *
 * @param fd
 * 		the file descriptor
 * @param off
 * 		the offset
 * @param-opt out_status
 * 		is filled with the status
 * @param whence
 * 		one of the {g_fs_seek_mode} constants
 *
 * @return if successful returns the new absolute offset, otherwise -1
 *
 * @security-level APPLICATION
 */
int64_t g_seek(g_fd fd, int64_t off, g_fs_seek_mode mode);
int64_t g_seek_s(g_fd fd, int64_t off, g_fs_seek_mode mode, g_fs_seek_status* out_status);

/**
 * Retrieves the current offset within a file.
 *
 * @param fd
 * 		the file descriptor
 * @param-opt out_status
 * 		is filled with the status
 *
 * @return if successful returns the current absolute offset, otherwise -1
 *
 * @security-level APPLICATION
 */
int64_t g_tell(g_fd fd);
int64_t g_tell_s(g_fd fd, g_fs_tell_status* out_status);

/**
 * Opens a pipe.
 *
 * @param out_write
 * 		is filled with the pipes write end
 * @param out_read
 * 		is filled with the pipes read end
 * @param out_status
 * 		is filled with the status code
 *
 * @security-level APPLICATION
 */
g_fs_pipe_status g_pipe(g_fd* out_write, g_fd* out_read);
g_fs_pipe_status g_pipe_b(g_fd* out_write, g_fd* out_read, g_bool blocking);

/**
 * Creates a mountpoint and registers the current thread as its file system delegate.
 *
 * @param name
 * 		the wanted name
 *
 * @param phys_mountpoint_id
 * 		the physical id to set for the mountpoint
 *
 * @param out_mountpoint_id
 * 		is filled with the node id of the mountpoint on success
 *
 * @param out_transaction_storage
 * 		is filled with the address of the transaction storage
 *
 * @return one of the {g_fs_register_as_delegate_status} codes
 *
 * @security-level DRIVER
 */
g_fs_register_as_delegate_status g_fs_register_as_delegate(const char* name, g_fs_phys_id phys_mountpoint_id,
                                                           g_fs_virt_id* out_mountpoint_id,
                                                           g_address* out_transaction_storage);

/**
 * Updates the status for a filesystem transaction.
 *
 * @param id
 * 		the transaction id
 *
 * @param status
 * 		the transaction status
 *
 * @security-level DRIVER
 */
void g_fs_set_transaction_status(g_fs_transaction_id id, g_fs_transaction_status status);

/**
 * Creates a filesystem node.
 *
 * @param parent
 * 		id of the parent node
 *
 * @param name
 * 		the node name
 *
 * @param type
 * 		one of the g_fs_node_type types
 *
 * @param phys_fs_id
 * 		the filesystem id of the node
 *
 * @param out_created_id
 * 		id of the created node
 *
 * @return one of the {g_fs_create_node_status} codes
 *
 * @security-level DRIVER
 */
g_fs_create_node_status g_fs_create_node(uint32_t parent, char* name, g_fs_node_type type, uint64_t fs_id,
                                         uint32_t* out_created_id);

/**
 * Clones a file descriptor in a process to another processes. Creates a new file
 * descriptor in the target process.
 *
 * @param source_fd
 * 		source file descriptor
 * @param source_pid
 * 		id of the source process
 * @param target_pid
 * 		id of the target process
 * @param-opt out_status
 * 		is filled with the status code
 *
 * @return the resulting file descriptor
 *
 * @security-level APPLICATION
 */
g_fd g_clone_fd(g_fd source_fd, g_pid source_process, g_pid target_process);
g_fd g_clone_fd_s(g_fd source_fd, g_pid source_process, g_pid target_process, g_fs_clonefd_status* out_status);

/**
 * Clones a file descriptor in a process to another processes file descriptor value.
 *
 * @param source_fd
 * 		source file descriptor
 * @param source_pid
 * 		id of the source process
 * @param target_fd
 * 		target file descriptor
 * @param target_pid
 * 		id of the target process
 * @param-opt out_status
 * 		is filled with the status code
 *
 * @return the target file descriptor
 *
 * @security-level APPLICATION
 */
g_fd g_clone_fd_t(g_fd source_fd, g_pid source_process, g_fd target_fd, g_pid target_process);
g_fd g_clone_fd_ts(g_fd source_fd, g_pid source_process, g_fd target_fd, g_pid target_process,
                   g_fs_clonefd_status* out_status);

/**
 * Resolves the path and returns the real path.
 *
 * @param in the input path
 * @param out pointer to a buffer where the real path is written
 * @return operation status
 *
 * @security-level APPLICATION
 */
g_fs_real_path_status g_real_path(const char* in, char* out);

/**
 * Retrieve stats of a node.
 *
 * @param path input path
 * @param out structure for the stat data
 * @param-opt follow_symlinks whether to follow symbolic links
 * @return whether the operation was successful
 *
 * @security-level APPLICATION
 */
g_fs_stat_status g_fs_stat(const char* path, g_fs_stat_data* out);
g_fs_stat_status g_fs_stat_l(const char* path, g_fs_stat_data* out, g_bool follow_symlinks);

/**
 * Retrieve stats of a node behind a file descriptor.
 *
 * @param fd file descriptor
 * @param out structure for the stat data
 * @return whether the operation was successful
 *
 * @security-level APPLICATION
 */
g_fs_stat_status g_fs_fstat(g_fd fd, g_fs_stat_data* out);

__END_C

#endif
