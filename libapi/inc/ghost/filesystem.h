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
 * Opens a file with flags set to 0.
 *
 * Equivalent to g_open_fs(path, 0, 0).
 *
 * @param path Null-terminated file path; relative paths use the working directory.
 * @return The opened file descriptor, or G_FD_NONE on failure.
 * @security-level APPLICATION
 */
g_fd g_open(const char* path);

/**
 * Opens a file with the specified flags.
 *
 * Equivalent to g_open_fs(path, flags, 0).
 *
 * @param path Null-terminated file path; relative paths use the working directory.
 * @param flags Bitwise OR of G_FILE_FLAG_MODE_* flags.
 * @return The opened file descriptor, or G_FD_NONE on failure.
 * @security-level APPLICATION
 */
g_fd g_open_f(const char* path, g_file_flag_mode flags);

/**
 * Opens a file and optionally reports the operation status.
 *
 * Close the returned descriptor with g_close() when it is no longer needed.
 *
 * @param path Null-terminated file path; relative paths use the working directory.
 * @param flags Bitwise OR of G_FILE_FLAG_MODE_* flags.
 * @param[out] out_status Receives a g_fs_open_status code; may be null.
 * @return The opened file descriptor, or G_FD_NONE on failure.
 * @security-level APPLICATION
 */
g_fd g_open_fs(const char* path, g_file_flag_mode flags, g_fs_open_status* out_status);

/**
 * Reads up to length bytes from a file descriptor into buffer.
 *
 * Equivalent to g_read_s(fd, buffer, length, 0).
 *
 * @param fd File descriptor to read from.
 * @param[out] buffer Destination buffer with space for at least length bytes.
 * @param length Maximum number of bytes to read; should not exceed INT32_MAX.
 * @return The number of bytes read, 0 at end of file, or -1 on failure.
 * @security-level APPLICATION
 */
int32_t g_read(g_fd fd, void* buffer, uint64_t length);

/**
 * Reads bytes from a file descriptor and optionally reports the operation status.
 *
 * A successful read may return fewer than length bytes. The descriptor offset
 * advances by the number of bytes read. A descriptor opened with
 * G_FILE_FLAG_MODE_BLOCKING waits while the filesystem reports G_FS_READ_BUSY;
 * otherwise, a busy read returns -1 with status G_FS_READ_BUSY.
 *
 * @param fd File descriptor to read from.
 * @param[out] buffer Destination buffer with space for at least length bytes.
 * @param length Maximum number of bytes to read; should not exceed INT32_MAX.
 * @param[out] out_status Receives a g_fs_read_status code; may be null.
 * @return The number of bytes read, 0 at end of file, or -1 on failure.
 * @security-level APPLICATION
 */
int32_t g_read_s(g_fd fd, void* buffer, uint64_t length, g_fs_read_status* out_status);

/**
 * Writes up to length bytes from buffer to a file descriptor.
 *
 * Equivalent to g_write_s(fd, buffer, length, 0).
 *
 * @param fd File descriptor to write to.
 * @param buffer Source buffer containing at least length bytes.
 * @param length Maximum number of bytes to write; should not exceed INT32_MAX.
 * @return The number of bytes written, or -1 on failure.
 * @security-level APPLICATION
 */
int32_t g_write(g_fd fd, const void* buffer, uint64_t length);

/**
 * Writes bytes to a file descriptor and optionally reports the operation status.
 *
 * A successful write may return fewer than length bytes. The descriptor offset
 * advances to the end of the written data. G_FILE_FLAG_MODE_APPEND starts each
 * write at the end of the file. G_FILE_FLAG_MODE_BLOCKING waits while the
 * filesystem reports G_FS_WRITE_BUSY; otherwise, a busy write returns -1 with
 * status G_FS_WRITE_BUSY.
 *
 * @param fd File descriptor to write to.
 * @param buffer Source buffer containing at least length bytes.
 * @param length Maximum number of bytes to write; should not exceed INT32_MAX.
 * @param[out] out_status Receives a g_fs_write_status code; may be null.
 * @return The number of bytes written, or -1 on failure.
 * @security-level APPLICATION
 */
int32_t g_write_s(g_fd fd, const void* buffer, uint64_t length, g_fs_write_status* out_status);

/**
 * Closes a file descriptor, including either end of a pipe.
 *
 * @param fd File descriptor to close.
 * @return A g_fs_close_status code.
 * @security-level APPLICATION
 */
g_fs_close_status g_close(g_fd fd);

/**
 * Retrieves the length of an open file in bytes.
 *
 * Equivalent to g_length_s(fd, 0). Use g_length_s() to detect failures.
 *
 * @param fd File descriptor to query.
 * @return The length in bytes on success; unspecified on failure.
 * @security-level APPLICATION
 */
int64_t g_length(g_fd fd);

/**
 * Retrieves the length of an open file and optionally reports the operation status.
 *
 * @param fd File descriptor to query.
 * @param[out] out_status Receives a g_fs_length_status code; may be null.
 * @return The length in bytes on success; unspecified on failure. Check out_status
 *         for G_FS_LENGTH_SUCCESSFUL before using the result.
 * @security-level APPLICATION
 */
int64_t g_length_s(g_fd fd, g_fs_length_status* out_status);

/**
 * Opens a directory for iteration.
 *
 * Equivalent to g_open_directory_s(path, 0).
 *
 * @param path Null-terminated directory path, resolved from the filesystem root.
 * @return An iterator to release with g_close_directory(), or null on failure.
 * @security-level APPLICATION
 */
g_fs_directory_iterator* g_open_directory(const char* path);

/**
 * Opens a directory and optionally reports the operation status.
 *
 * Iteration starts at the first entry. Directory paths are currently resolved
 * from the filesystem root, even when they do not begin with '/'.
 *
 * @param path Null-terminated directory path, resolved from the filesystem root.
 * @param[out] out_status Receives a g_fs_open_directory_status code; may be null.
 * @return An iterator to release with g_close_directory(), or null on failure.
 * @security-level APPLICATION
 */
g_fs_directory_iterator* g_open_directory_s(const char* path, g_fs_open_directory_status* out_status);

/**
 * Reads the next directory entry and advances the iterator.
 *
 * Equivalent to g_read_directory_s(iterator, 0). See g_read_directory_s() for
 * entry ownership and lifetime.
 *
 * @param[in,out] iterator A valid iterator returned by g_open_directory().
 * @return An entry owned by the iterator, or null at end of directory or on error.
 * @security-level APPLICATION
 */
g_fs_directory_entry* g_read_directory(g_fs_directory_iterator* iterator);

/**
 * Reads the next directory entry and optionally reports the operation status.
 *
 * The entry and its name belong to the iterator. Their contents are overwritten
 * by the next successful read and freed by g_close_directory(); do not free them.
 * Copy any data that must be retained across reads.
 *
 * @param[in,out] iterator A valid iterator returned by g_open_directory().
 * @param[out] out_status Receives a g_fs_read_directory_status code; may be null.
 *                       G_FS_READ_DIRECTORY_EOD indicates end of directory.
 * @return An entry owned by the iterator, or null at end of directory or on error.
 * @security-level APPLICATION
 */
g_fs_directory_entry* g_read_directory_s(g_fs_directory_iterator* iterator, g_fs_read_directory_status* out_status);

/**
 * Frees a directory iterator and its entry buffer, including the entry name.
 *
 * @param iterator A non-null iterator returned by g_open_directory(). The iterator
 *                 and all pointers into its entry buffer become invalid.
 * @security-level APPLICATION
 */
void g_close_directory(g_fs_directory_iterator* iterator);

/**
 * Repositions the offset within an open file.
 *
 * Equivalent to g_seek_s(fd, off, mode, 0). See g_seek_s() for seek mode behavior;
 * use it with a status output to detect failures.
 *
 * @param fd File descriptor to reposition.
 * @param off Byte offset interpreted according to mode.
 * @param mode One of G_FS_SEEK_SET, G_FS_SEEK_CUR, or G_FS_SEEK_END.
 * @return The new absolute offset on success; unspecified on failure.
 * @security-level APPLICATION
 */
int64_t g_seek(g_fd fd, int64_t off, g_fs_seek_mode mode);

/**
 * Repositions a file offset and optionally reports the operation status.
 *
 * G_FS_SEEK_SET uses off as the absolute offset. G_FS_SEEK_CUR adds off to the
 * current offset. The kernel currently interprets G_FS_SEEK_END as file length
 * minus off, so positive values move backwards from the end. Offsets above the
 * file length are clamped to the file length.
 *
 * @param fd File descriptor to reposition.
 * @param off Byte offset interpreted according to mode.
 * @param mode One of G_FS_SEEK_SET, G_FS_SEEK_CUR, or G_FS_SEEK_END.
 * @param[out] out_status Receives a g_fs_seek_status code; may be null.
 * @return The new absolute offset on success; unspecified on failure. Check
 *         out_status for G_FS_SEEK_SUCCESSFUL before using the result.
 * @security-level APPLICATION
 */
int64_t g_seek_s(g_fd fd, int64_t off, g_fs_seek_mode mode, g_fs_seek_status* out_status);

/**
 * Retrieves the current offset within an open file.
 *
 * Equivalent to g_tell_s(fd, 0).
 *
 * @param fd File descriptor to query.
 * @return The current absolute offset in bytes, or -1 if fd is invalid.
 * @security-level APPLICATION
 */
int64_t g_tell(g_fd fd);

/**
 * Retrieves a file offset and optionally reports the operation status.
 *
 * @param fd File descriptor to query.
 * @param[out] out_status Receives a g_fs_tell_status code; may be null.
 * @return The current absolute offset in bytes, or -1 if fd is invalid.
 * @security-level APPLICATION
 */
int64_t g_tell_s(g_fd fd, g_fs_tell_status* out_status);

/**
 * Creates a pipe with blocking reads and writes.
 *
 * Equivalent to g_pipe_b(out_write, out_read, true).
 *
 * @param[out] out_write Non-null pointer receiving the write descriptor on success.
 * @param[out] out_read Non-null pointer receiving the read descriptor on success.
 * @return A g_fs_pipe_status code.
 * @security-level APPLICATION
 */
g_fs_pipe_status g_pipe(g_fd* out_write, g_fd* out_read);

/**
 * Creates a pipe with configurable write blocking.
 *
 * Reads always block while the pipe is empty and a writer remains open. If
 * blocking is true, writes wait for space; otherwise, they return -1 with
 * G_FS_WRITE_BUSY when space is insufficient. Successful writes can be partial.
 * Close both descriptors with g_close() when they are no longer needed.
 *
 * @param[out] out_write Non-null pointer receiving the write descriptor on success.
 * @param[out] out_read Non-null pointer receiving the read descriptor on success.
 * @param blocking Whether writes wait for space in the pipe.
 * @return A g_fs_pipe_status code. Use the outputs only on G_FS_PIPE_SUCCESSFUL.
 * @security-level APPLICATION
 */
g_fs_pipe_status g_pipe_b(g_fd* out_write, g_fd* out_read, g_bool blocking);

/**
 * Creates a mountpoint and registers the current thread as its filesystem delegate.
 *
 * @note This legacy system call has no handler in the current kernel; its return
 *       value and outputs are unspecified.
 *
 * @param name Null-terminated mountpoint name.
 * @param phys_mountpoint_id Physical filesystem identifier for the mountpoint.
 * @param[out] out_mountpoint_id Non-null pointer receiving the virtual mountpoint
 *                              identifier on success.
 * @param[out] out_transaction_storage Non-null pointer receiving the transaction
 *                                    storage address on success.
 * @return A g_fs_register_as_delegate_status code when supported.
 * @security-level DRIVER
 */
g_fs_register_as_delegate_status g_fs_register_as_delegate(const char* name, g_fs_phys_id phys_mountpoint_id,
                                                           g_fs_virt_id* out_mountpoint_id,
                                                           g_address* out_transaction_storage);

/**
 * Updates the status of a filesystem transaction.
 *
 * @note This legacy system call has no handler in the current kernel and has no
 *       effect.
 *
 * @param id Transaction identifier.
 * @param status One of G_FS_TRANSACTION_WAITING, G_FS_TRANSACTION_FINISHED, or
 *               G_FS_TRANSACTION_REPEAT.
 * @security-level DRIVER
 */
void g_fs_set_transaction_status(g_fs_transaction_id id, g_fs_transaction_status status);

/**
 * Creates or updates a node in the virtual filesystem.
 *
 * @note This legacy system call has no handler in the current kernel; its return
 *       value and outputs are unspecified.
 *
 * @param parent Virtual identifier of the parent node.
 * @param name Null-terminated node name.
 * @param type One of the G_FS_NODE_TYPE_* constants.
 * @param fs_id Physical filesystem identifier of the node.
 * @param[out] out_created_id Non-null pointer receiving the virtual identifier of
 *                           the created or updated node on success.
 * @return A g_fs_create_node_status code when supported.
 * @security-level DRIVER
 */
g_fs_create_node_status g_fs_create_node(uint32_t parent, char* name, g_fs_node_type type, uint64_t fs_id,
                                         uint32_t* out_created_id);

/**
 * Clones a file descriptor into a newly allocated descriptor in a target process.
 *
 * Equivalent to g_clone_fd_ts(source_fd, source_process, G_FD_NONE, target_process,
 * 0). Use g_clone_fd_s() to detect failures.
 *
 * @param source_fd File descriptor in source_process.
 * @param source_process Source process identifier.
 * @param target_process Target process identifier; may equal source_process.
 * @return The new descriptor in target_process on success; unspecified on failure.
 * @security-level APPLICATION
 */
g_fd g_clone_fd(g_fd source_fd, g_pid source_process, g_pid target_process);

/**
 * Clones a file descriptor and optionally reports the operation status.
 *
 * Allocates a new descriptor in target_process. The clone copies the source open
 * flags and current offset; subsequent changes to either offset are independent.
 *
 * @param source_fd File descriptor in source_process.
 * @param source_process Source process identifier.
 * @param target_process Target process identifier; may equal source_process.
 * @param[out] out_status Receives a g_fs_clonefd_status code; may be null.
 * @return The new descriptor in target_process on success; unspecified on failure.
 *         Check out_status for G_FS_CLONEFD_SUCCESSFUL before using the result.
 * @security-level APPLICATION
 */
g_fd g_clone_fd_s(g_fd source_fd, g_pid source_process, g_pid target_process, g_fs_clonefd_status* out_status);

/**
 * Clones a file descriptor into a specified descriptor in a target process.
 *
 * Equivalent to g_clone_fd_ts(source_fd, source_process, target_fd, target_process,
 * 0). See g_clone_fd_ts() for replacement behavior and failure reporting.
 *
 * @param source_fd File descriptor in source_process.
 * @param source_process Source process identifier.
 * @param target_fd Target descriptor to replace, or G_FD_NONE to allocate a new one.
 * @param target_process Target process identifier; may equal source_process.
 * @return The resulting descriptor in target_process on success; unspecified on
 *         failure.
 * @security-level APPLICATION
 */
g_fd g_clone_fd_t(g_fd source_fd, g_pid source_process, g_fd target_fd, g_pid target_process);

/**
 * Clones a file descriptor into a target process and optionally reports status.
 *
 * If target_fd is G_FD_NONE, a new descriptor is allocated. Otherwise, the source
 * is validated before closing the existing target descriptor. Cloning a
 * descriptor onto itself succeeds without closing it. Other failures after
 * closing the target leave it closed. The clone copies the source open flags
 * and offset; subsequent changes to either offset are independent.
 *
 * @param source_fd File descriptor in source_process.
 * @param source_process Source process identifier.
 * @param target_fd Target descriptor to replace, or G_FD_NONE to allocate a new one.
 * @param target_process Target process identifier; may equal source_process.
 * @param[out] out_status Receives a g_fs_clonefd_status code; may be null.
 * @return The resulting descriptor in target_process on success; unspecified on
 *         failure. Check out_status for G_FS_CLONEFD_SUCCESSFUL before using it.
 * @security-level APPLICATION
 */
g_fd g_clone_fd_ts(g_fd source_fd, g_pid source_process, g_fd target_fd, g_pid target_process,
                   g_fs_clonefd_status* out_status);

/**
 * Resolves an existing filesystem node to its absolute path.
 *
 * @param in Null-terminated path; relative paths use the working directory.
 * @param[out] out Buffer of at least G_PATH_MAX bytes for the null-terminated path.
 * @return A g_fs_real_path_status code. Use out only on G_FS_REAL_PATH_SUCCESS.
 * @security-level APPLICATION
 */
g_fs_real_path_status g_real_path(const char* in, char* out);

/**
 * Retrieves filesystem metadata for a path.
 *
 * Equivalent to g_fs_stat_l(path, out, true). See g_fs_stat_l() for current
 * metadata and symbolic-link limitations.
 *
 * @param path Null-terminated path; relative paths use the working directory.
 * @param[out] out Receives the node metadata on success.
 * @return A g_fs_stat_status code, with G_FS_STAT_SUCCESS indicating success.
 * @security-level APPLICATION
 */
g_fs_stat_status g_fs_stat(const char* path, g_fs_stat_data* out);

/**
 * Retrieves filesystem metadata with a requested symbolic-link policy.
 *
 * The kernel currently ignores follow_symlinks. It reports device as -1 and all
 * timestamps as 0. If the file length cannot be retrieved, size is (uint64_t) -1;
 * this does not cause the stat operation to fail.
 *
 * @param path Null-terminated path; relative paths use the working directory.
 * @param[out] out Receives the node metadata on success.
 * @param follow_symlinks Whether to follow symbolic links; currently ignored.
 * @return A g_fs_stat_status code, with G_FS_STAT_SUCCESS indicating success.
 * @security-level APPLICATION
 */
g_fs_stat_status g_fs_stat_l(const char* path, g_fs_stat_data* out, g_bool follow_symlinks);

/**
 * Retrieves filesystem metadata for the node referenced by an open descriptor.
 *
 * The metadata field limitations documented for g_fs_stat_l() also apply here.
 *
 * @param fd File descriptor to query.
 * @param[out] out Receives the node metadata on success.
 * @return A g_fs_stat_status code, with G_FS_STAT_SUCCESS indicating success.
 * @security-level APPLICATION
 */
g_fs_stat_status g_fs_fstat(g_fd fd, g_fs_stat_data* out);

__END_C

#endif
