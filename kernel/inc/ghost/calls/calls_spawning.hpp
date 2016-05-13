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

#ifndef GHOST_API_CALLS_SPAWNINGCALLS
#define GHOST_API_CALLS_SPAWNINGCALLS

#include "ghost/kernel.h"
#include "ghost/system.h"
#include "ghost/ramdisk.h"

/**
 * @field path
 * 		absolute path of the binary
 *
 * @field securityLevel
 * 		target process security level
 *
 * @field spawnStatus
 * 		result of spawning
 */
typedef struct {
	char* path;
	g_security_level securityLevel;

	g_ramdisk_spawn_status spawnStatus;
}__attribute__((packed)) g_syscall_ramdisk_spawn;

/**
 * @field initialEntry
 * 		the initial thread entry
 *
 * @field userEntry
 * 		user-defined
 *
 * @field userData
 * 		user-defined
 *
 * @field status
 * 		result of thread creation
 */
typedef struct {
	void* initialEntry;
	void* userEntry;
	void* userData;

	g_create_thread_status status;
	g_tid threadId;
}__attribute__((packed)) g_syscall_create_thread;

/**
 * @field userEntry
 * 		the user entry
 *
 * @field userData
 * 		the user data
 */
typedef struct {
	void* userEntry;
	void* userData;
}__attribute__((packed)) g_syscall_get_thread_entry;

/**
 * @field securityLevel
 * 		the security level to apply
 *
 * @field processObject
 * 		a handle to the created process, or 0 if the
 * 		creation has failed
 */
typedef struct {
	g_security_level securityLevel;

	g_process_creation_identifier processObject;
}__attribute__((packed)) g_syscall_create_empty_process;

/**
 * @field processObject
 * 		handle to the target process
 *
 * @field targetSpaceVirtualAddress
 * 		virtual address in the target space to map the pages to
 *
 * @field numberOfPages
 * 		number of pages to map
 *
 * @field resultVirtualAddress
 * 		the virtual address of the mapped area in the current space
 */
typedef struct {
	g_process_creation_identifier processObject;
	uint32_t targetSpaceVirtualAddress;
	uint32_t numberOfPages;

	uint32_t resultVirtualAddress;
}__attribute__((packed)) g_syscall_create_pages_in_space;

/**
 * @field processObject
 * 		handle to the target process
 *
 * @field content
 * 		contents to copy to tls master copy
 *
 * @field copysize
 * 		number of bytes to copy from content
 *
 * @field totalsize
 * 		number of bytes the tls is in total (including bytes to zero)
 *
 * @field alignment
 * 		tls alignment
 *
 * @field result
 * 		whether mapping was successful
 */
typedef struct {
	g_process_creation_identifier processObject;
	uint8_t* content;
	uint32_t copysize;
	uint32_t totalsize;
	uint32_t alignment;

	uint8_t result;
}__attribute__((packed)) g_syscall_write_tls_master_for_process;

/**
 * Used for process configuration on spawning.
 */
typedef struct {
	char* source_path;
}__attribute__((packed)) g_process_configuration;

/**
 * @field processObject
 * 		handle to the target process
 *
 * @field configuration
 * 		configuration content
 *
 * @field result
 * 		whether configuration was successful
 */
typedef struct {
	g_process_creation_identifier processObject;
	g_process_configuration configuration;

	uint8_t result;
}__attribute__((packed)) g_syscall_configure_process;

/**
 * @field eip
 * 		the start instruction address
 *
 * @field processObject
 * 		handle to the target process
 */
typedef struct {
	uint32_t eip;
	g_process_creation_identifier processObject;
}__attribute__((packed)) g_syscall_attach_created_process;

/**
 * @field processObject
 * 		handle to the target process
 */
typedef struct {
	g_process_creation_identifier processObject;
}__attribute__((packed)) g_syscall_cancel_process_creation;

/**
 * @field processObject
 * 		handle to the target process
 *
 * @field resultId
 * 		the resulting process id
 */
typedef struct {
	g_process_creation_identifier processObject;

	g_pid resultId;
}__attribute__((packed)) g_syscall_get_created_process_id;

/**
 * @field processObject
 * 		handle to the target process
 *
 * @field arguments
 * 		source buffer, with a size of
 * 		{PROCESS_COMMAND_LINE_ARGUMENTS_BUFFER_LENGTH}
 */
typedef struct {
	g_process_creation_identifier processObject;
	char* arguments;
}__attribute__((packed)) g_syscall_cli_args_store;

/**
 * @field buffer
 * 		target buffer, with a size of at least
 * 		{PROCESS_COMMAND_LINE_ARGUMENTS_BUFFER_LENGTH}
 */
typedef struct {
	char* buffer;
}__attribute__((packed)) g_syscall_cli_args_release;

#endif
