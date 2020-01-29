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

#ifndef __GHOST_USER__
#define __GHOST_USER__

#include <stdarg.h>
#include <stddef.h>
#include "ghost/types.h"
#include "ghost/common.h"
#include "ghost/kernel.h"
#include "ghost/system.h"
#include "ghost/ramdisk.h"
#include "ghost/ipc.h"
#include "ghost/fs.h"
#include "ghost/calls/calls.h"

__BEGIN_C

/**
 * Performs an atomic wait. If the atom is true, the executing task must
 * wait until the task that owns the atom has finished its work and sets
 * it to false. Once the atom is false, it is set to true and the function
 * returns.
 *
 * @param atom
 * 		the atom to use
 *
 * @security-level APPLICATION
 */
void g_atomic_lock(g_atom* atom);
void g_atomic_lock_dual(g_atom* a1, g_atom* a2);
g_bool g_atomic_lock_to(g_atom* atom, uint64_t timeout);
g_bool g_atomic_lock_dual_to(g_atom* a1, g_atom* a2, uint64_t timeout);

/**
 * Trys to perform atomic wait. If the lock is already locked, the function
 * returns 0. Otherwise, the lock is set as in {g_atomic_lock} and the
 * function returns 1.
 *
 * @param atom
 * 		the atom to use
 *
 * @security-level APPLICATION
 */
g_bool g_atomic_try_lock(g_atom* atom);
g_bool g_atomic_try_lock_dual(g_atom* a1, g_atom* a2);
g_bool g_atomic_try_lock_to(g_atom* atom, uint64_t timeout);
g_bool g_atomic_try_lock_dual_to(g_atom* a1, g_atom* a2, uint64_t timeout);

/**
 * Performs an atomic block. If the atom is true, the executing task must
 * wait until the task that owns the atom has finished its work and sets
 * it to false. Different from the {g_atomic_lock}, the atom is not changed.
 *
 * @param atom
 * 		the atom to use
 *
 * @security-level APPLICATION
 */
void g_atomic_block(g_atom* atom);
void g_atomic_block_dual(g_atom* a1, g_atom* a2);
g_bool g_atomic_block_to(g_atom* atom, uint64_t timeout);
g_bool g_atomic_block_dual_to(g_atom* a1, g_atom* a2, uint64_t timeout);

/**
 * Spawns a program binary.
 *
 * @param path absolute path of the executable
 * @param args unparsed arguments
 * @param workdir working directory for the execution
 * @param securityLevel security level to spawn the process to
 * @param outProcessId is filled with the process id
 * @param outStdio is filled with stdio file descriptors, 0 is write end of stdin,
 * 		1 is read end of stdout, 2 is read end of stderr
 * @param inStdio if supplied, the given descriptors which are valid for the executing process
 * 		are used as the stdin/out/err for the spawned process; an entry might be -1
 * 		to be ignored and default behaviour being applied
 *
 * @return one of the {g_spawn_status} codes
 *
 * @security-level APPLICATION
 */
g_spawn_status g_spawn(const char* path, const char* args, const char* workdir, g_security_level securityLevel);
g_spawn_status g_spawn_p(const char* path, const char* args, const char* workdir, g_security_level securityLevel, g_pid* pid);
g_spawn_status g_spawn_po(const char* path, const char* args, const char* workdir, g_security_level securityLevel, g_pid* pid, g_fd outStdio[3]);
g_spawn_status g_spawn_poi(const char* path, const char* args, const char* workdir, g_security_level securityLevel, g_pid* pid, g_fd outStdio[3], g_fd inStdio[3]);
g_spawn_status g_spawn_poid(const char* path, const char* args, const char* workdir, g_security_level securityLevel, g_pid* pid, g_fd outStdio[3], g_fd inStdio[3], g_spawn_validation_details* outValidationDetails);

/**
 * Kills a process.
 *
 * @param pid
 * 		the process id
 *
 * @security-level APPLICATION
 */
g_kill_status g_kill(g_pid pid);

/**
 * Prints a message to the log.
 *
 * @param message
 * 		the message to log
 *
 * @security-level APPLICATION
 */
void g_log(const char* message);

/**
 * Performs the software interrupt necessary for the system call passing the
 * given data (usually a pointer to a call struct).
 *
 * @param call
 * 		the call to execute
 * @param data
 * 		the data to pass
 *
 * Clobber flags (more: https://gcc.gnu.org/onlinedocs/gcc/Extended-Asm.html):
 * - The "cc" clobber indicates that the assembler code modifies the FLAGS register
 * - The "memory" clobber tells the compiler that the assembly code performs memory reads
 *   or writes to items other than those listed in the input and output operands. Causes
 *   GCC to assume that anything in memory could have changed (the system call could do so).
 *
 * @security-level APPLICATION
 */
void g_syscall(uint32_t call, uint32_t data);

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
 * Retrieves the length of a file in bytes.
 *
 * @param path
 * 		path of the file
 *
 * @param follow_symlinks
 * 		whether to follow symbolic links or not
 *
 * @param-opt out_status
 * 		is filled with the status code
 *
 * @return the length in bytes
 *
 * @security-level APPLICATION
 */
int64_t g_flength(const char* path);
int64_t g_flength_s(const char* path, uint8_t follow_symlinks);
int64_t g_flength_ss(const char* path, uint8_t follow_symlinks, g_fs_length_status* out_status);

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
 * Sets the working directory for the current process.
 *
 * @param path
 * 		buffer of at least {G_PATH_MAX} bytes size
 * 		containing the new working directory
 * 
 * @return one of the {g_set_working_directory_status} codes
 *
 * @security-level APPLICATION if no process given, otherwise KERNEL
 */
g_set_working_directory_status g_set_working_directory(const char* path);

/**
 * Retrieves the working directory for the current process.
 *
 * @param path
 * 		buffer of at least <maxlen> or {G_PATH_MAX} bytes size
 *
 * @param maxlen
 * 		length of the buffer in bytes
 *
 * @return whether the action was successful
 *
 * @security-level APPLICATION
 */
g_get_working_directory_status g_get_working_directory(char* buffer);
g_get_working_directory_status g_get_working_directory_l(char* buffer, size_t maxlen);

/**
 * Sets the working directory for the current process.
 * 
 * @param path
 * 		buffer of at least <maxlen> or {G_PATH_MAX} bytes size
 *
 * @return whether the action was successful
 *
 * @security-level APPLICATION
 */
g_set_working_directory_status g_set_working_directory(const char* path);

/**
 * Retrieves the directory of the executable when available, otherwise an empty
 * string is written to the buffer.
 *
 * @param path
 * 		buffer of at least {G_PATH_MAX} bytes size
 *
 * @security-level APPLICATION
 */
void g_get_executable_path(char* buffer);

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
 * Returns the next transaction id that can be used for messaging.
 * When sending a message, a transaction can be added so that one can wait
 * for an answer with the same transaction. This method always returns a
 * new transaction id each time it is called and is thread-safe.
 *
 * @return a new transaction id
 *
 * @security-level APPLICATION
 */
g_message_transaction g_get_message_tx_id();

/**
 * Allocates a memory region with the size of at least the given
 * size in bytes. This region can for example be used as shared memory.
 *
 * Allocating memory using this call makes the requesting process the physical owner of the
 * pages in its virtual space (important for unmapping).
 *
 * @param size
 * 		the size in bytes
 *
 * @return a pointer to the allocated memory region, or 0 if failed
 *
 * @security-level APPLICATION
 */
void* g_alloc_mem(int32_t size);

/**
 * Shares a memory area with another process.
 *
 * @param memory
 * 		a pointer to the memory area to share
 * @param size
 * 		the size of the memory area
 * @param pid
 * 		the id of the target process
 *
 * @return a pointer to the shared memory location within the target address space
 *
 * @security-level APPLICATION
 */
void* g_share_mem(void* memory, int32_t size, g_pid pid);

/**
 * Yields, causing a switch to the next process.
 *
 * @security-level APPLICATION
 */
void g_yield();

/**
 * Sleeps for the given amount of milliseconds.
 *
 * @param ms
 * 		the milliseconds to sleep
 *
 * @security-level APPLICATION
 */
void g_sleep(uint64_t ms);

/**
 * Retrieves the current process id.
 *
 * @return the id of the executing process
 *
 * @security-level APPLICATION
 */
g_pid g_get_pid();

/**
 * Retrieves the id of the parent process for a given process.
 *
 * @param pid
 * 		id of the process
 *
 * @return the id of the executing process
 *
 * @security-level APPLICATION
 */
g_pid g_get_parent_pid(g_pid pid);

/**
 * Retrieves the current thread id. If this thread is the main
 * thread of the process, the id is the same as the process id.
 *
 * @return the id of the executing thread
 *
 * @security-level APPLICATION
 */
g_tid g_get_tid();

/**
 * Retrieves the process id for a thread id.
 *
 * @param tid the thread id
 *
 * @return the process id of the thread, or -1
 *
 * @security-level APPLICATION
 */
g_pid g_get_pid_for_tid(uint32_t tid);

/**
 * Quits the process with the given status code.
 *
 * @param status
 * 		the status code
 *
 * @security-level APPLICATION
 */
void g_exit(int status);

/**
 * Creates a thread.
 *
 * @param function
 * 		the entry function
 * @param-opt userData
 * 		a pointer to user data that should be passed
 * 		to the entry function
 *
 * @security-level APPLICATION
 */
g_tid g_create_thread(void* function);
g_tid g_create_thread_d(void* function, void* userData);
g_tid g_create_thread_ds(void* function, void* userData, g_create_thread_status* out_status);

/**
 * Sends a message to the given task. This means that <len> bytes from the
 * buffer <buf> are copied to a message that is then sent to the <target> task.
 * The message may be no longer than {G_MESSAGE_MAXIMUM_LENGTH}.
 *
 * The mode specifies how the function shall block:
 * - {G_MESSAGE_SEND_MODE_BLOCKING} the executing task will bock if the target tasks
 * 		message queue is full
 * - {G_MESSAGE_SEND_MODE_NON_BLOCKING} the function will return {G_MESSAGE_SEND_STATUS_QUEUE_FULL}
 * 		if the target tasks message queue is full
 *
 * @param target
 * 		id of the target task
 * @param buf
 * 		message content buffer
 * @param len
 * 		number of bytes to copy from the buffer
 * @param-opt mode
 * 		determines how the function blocks when given, default is {G_MESSAGE_SEND_MODE_BLOCKING}
 * @param-opt tx
 * 		transaction id
 *
 * @return one of the <g_message_send_status> codes
 *
 * @security-level APPLICATION
 */
g_message_send_status g_send_message(g_tid target, void* buf, size_t len);
g_message_send_status g_send_message_m(g_tid target, void* buf, size_t len, g_message_send_mode mode);
g_message_send_status g_send_message_t(g_tid tid, void* buf, size_t len, g_message_transaction tx);
g_message_send_status g_send_message_tm(g_tid tid, void* buf, size_t len, g_message_transaction tx, g_message_send_mode mode);

/**
 * Receives a message. At maximum <max> bytes will be attempted to be copied to
 * the buffer <buf>. Note that when receiving a message, a buffer with a size of
 * at least the size of {g_message_header} plus the size of the sent message
 * must be used.
 *
 * After successful completion, the buffer will contain the message header followed
 * by the content of the message.
 * - to access the header, use the buffer pointer: ((g_message_header*) buf)
 * - to access the content, use the helper macro:  G_MESSAGE_CONTENT(buf)
 *
 * The mode specifies how the function shall block:
 * - {G_MESSAGE_RECEIVE_MODE_BLOCKING} the executing task will block if no messages
 * 		are available
 * - {G_MESSAGE_RECEIVE_MODE_NON_BLOCKING} the function will return {G_MESSAGE_RECEIVE_STATUS_QUEUE_EMPTY}
 * 		if the executing tasks message queue is empty
 *
 * When a transaction ID is given, only messages that were sent with the same
 * transaction ID will be received.
 *
 * @param buf
 * 		output buffer
 * @param max
 * 		maximum number of bytes to copy to the buffer
 * @param-opt mode
 * 		determines how the function blocks when given, default is {G_MESSAGE_RECEIVE_MODE_BLOCKING}
 * @param-opt tx
 * 		transaction id
 * @param break_condition
 * 		can be used to break the waiting process by setting its value to 1
 *
 * @security-level APPLICATION
 */
g_message_receive_status g_receive_message(void* buf, size_t max);
g_message_receive_status g_receive_message_m(void* buf, size_t max, g_message_receive_mode mode);
g_message_receive_status g_receive_message_t(void* buf, size_t max, g_message_transaction tx);
g_message_receive_status g_receive_message_tm(void* buf, size_t max, g_message_transaction tx, g_message_receive_mode mode);
g_message_receive_status g_receive_message_tmb(void* buf, size_t max, g_message_transaction tx, g_message_receive_mode mode, uint8_t* break_condition);

/**
 * Registers the executing task for the given identifier.
 *
 * @param identifier
 * 		the identifier to set
 *
 * @return if it was successful true, if the identifier is taken false
 *
 * @security-level APPLICATION
 */
uint8_t g_task_register_id(const char* identifier);

/**
 * Returns the id of the task that is register for the given identifier.
 *
 * @param identifier
 * 		the identifier
 *
 * @return the id of the task, or -1 if no task has this identifier
 *
 * @security-level APPLICATION
 */
g_tid g_task_get_id(const char* identifier);

/**
 * Maps the given physical address to the executing processes address space so
 * it can access it directly.
 *
 * @param addr
 * 		the physical memory address that should be mapped
 * @param size
 * 		the size that should be mapped
 *
 * @return a pointer to the mapped area within the executing processes address space
 *
 * @security-level DRIVER
 */
void* g_map_mmio(void* addr, uint32_t size);

/**
 * Unmaps the given memory area.
 *
 * @param area
 * 		a pointer to the area
 *
 * @security-level DRIVER
 */
void g_unmap(void* area);

/**
 * Adjusts the program heap break.
 *
 * @param amount
 * 		the value to adjust the break by
 * @param out_brk	is filled with the result
 *
 * @return whether adjusting was successful
 *
 * @security-level APPLICATION
 */
uint8_t g_sbrk(int amount, void** out_brk);

/**
 * Performs a Virtual 8086 BIOS interrupt call.
 *
 * @param interrupt
 * 		number of the interrupt to fire
 * @param in
 * 		input register values
 * @param out
 * 		output register values
 *
 * @return one of the G_VM86_CALL_STATUS_* status codes
 *
 * @security-level DRIVER
 */
g_vm86_call_status g_call_vm86(uint32_t interrupt, g_vm86_registers* in, g_vm86_registers* out);

/**
 * Frees a memory area allocated with {g_lower_malloc}.
 *
 * @param memory
 * 		a pointer to the memory area
 *
 * @security-level DRIVER
 */
void g_lower_free(void* memory);

/**
 * Allocates a memory area in the lower 1MiB. This can be used
 * for example for the virtual 8086 mode.
 *
 * @param size
 * 		the size to allocate
 *
 * @security-level DRIVER
 */
void* g_lower_malloc(uint32_t size);

/**
 * Enables or disables logging to the video output.
 *
 * @param enabled
 * 		whether to enable/disable video log
 *
 * @security-level APPLICATION
 */
void g_set_video_log(uint8_t enabled);

/**
 * TODO: currently returns the number of milliseconds that one
 * of the schedulers is running.
 *
 * @return the number of milliseconds
 *
 * @security-level APPLICATION
 */
uint64_t g_millis();

/**
 * Test-call for kernel debugging.
 *
 * @security-level VARIOUS
 */
uint32_t g_test(uint32_t test);

/**
 * Forks the current process. Only works from the main thread.
 *
 * @return within the executing process the forked processes id is returned,
 * 		within the forked process 0 is returned
 *
 * @security-level APPLICATION
 */
g_tid g_fork();

/**
 * Joins the task with the given id, making the executing task
 * wait until this task has died.
 *
 * @param tid
 * 		id of the task to join
 *
 * @security-level APPLICATION
 */
void g_join(g_tid tid);

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
g_fd g_clone_fd_ts(g_fd source_fd, g_pid source_process, g_fd target_fd, g_pid target_process, g_fs_clonefd_status* out_status);

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
g_fs_register_as_delegate_status g_fs_register_as_delegate(const char* name, g_fs_phys_id phys_mountpoint_id, g_fs_virt_id* out_mountpoint_id,
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
g_fs_create_node_status g_fs_create_node(uint32_t parent, char* name, g_fs_node_type type, uint64_t fs_id, uint32_t* out_created_id);

/**
 * Registers the <handler> routine as the handler for the <irq>.
 *
 * @param irq
 * 		IRQ number to handle
 *
 * @param handler
 * 		handler routine to call
 *
 * @return one of the {g_register_irq_handler_status} codes
 *
 * @security-level DRIVER
 */
g_register_irq_handler_status g_register_irq_handler(uint8_t irq, void (*handler)(uint8_t));

/**
 * Restores the interruption state (for example after signal/irq handling) of the current thread.
 */
void g_restore_interrupted_state();

/**
 * Registers the <handler> routine as the handler for the <irq>.
 *
 * @param signal
 * 		signal to handle
 *
 * @param handler
 * 		handler routine to call
 *
 * @return
 * 		previously registered handler
 *
 * @security-level DRIVER
 */
void* g_register_signal_handler(int signal, void(*handler)(int));

/**
 * Raises the <signal> in the <process>.
 *
 * @param process
 * 		target process
 *
 * @param signal
 * 		signal number
 *
 * @return one of the {g_raise_signal_status} codes
 */
g_raise_signal_status g_raise_signal(g_pid process, int signal);

/**
 * Executes the given kernquery.
 *
 * @param command
 * 		query command
 *
 * @param buffer
 * 		communication buffer
 *
 * @return one of the {g_kernquery_status} codes
 */
g_kernquery_status g_kernquery(uint16_t command, uint8_t* buffer);

/**
 * Returns and releases the command line arguments for the executing process.
 * This buffer must have a length of at least {PROCESS_COMMAND_LINE_ARGUMENTS_BUFFER_LENGTH} bytes.
 * If no arguments were supplied for the executing process, the buffer is null-terminated only.
 *
 * @param buffer
 * 		target buffer to store the arguments to
 *
 * @security-level KERNEL
 */
void g_cli_args_release(char* buffer);

/**
 * @return a pointer to the user-thread object in the TLS of the current thread.
 */
void* g_task_get_tls();

/**
 * Returns a pointer to the process information structure. This structure for example
 * contains a list of all loaded objects.
 * 
 * @return a pointer to a g_process_info
 */
g_process_info* g_process_get_info();

__END_C

#endif
