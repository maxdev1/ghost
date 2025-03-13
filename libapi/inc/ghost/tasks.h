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

#ifndef GHOST_API_TASKS
#define GHOST_API_TASKS

#include "common.h"
#include "tasks/types.h"

__BEGIN_C
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
 * Creates a thread.
 *
 * @param function
 * 		the entry function
 * @param-opt userData
 * 		a pointer to user data that should be passed
 * 		to the entry function
 * @param-opt coreAffinity
 *      core affinity of this task, use G_TASK_CORE_AFFINITY_NONE for any
 * @param-opt outStatus
 *      outputs the status
 *
 * @security-level APPLICATION
 */
g_tid g_create_task(void* function);
g_tid g_create_task_d(void* function, void* userData);
g_tid g_create_task_a(void* function, uint8_t coreAffinity);
g_tid g_create_task_ds(void* function, void* userData, g_create_task_status* outStatus);
g_tid g_create_task_da(void* function, void* userData, uint8_t coreAffinity);
g_tid g_create_task_das(void* function, void* userData, uint8_t coreAffinity, g_create_task_status* outStatus);

/**
 * Exits a thread.
 *
 * @security-level APPLICATION
 */
void g_exit_task();


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
 * Returns the id of the task that is registered for the given identifier.
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
g_spawn_status g_spawn_p(const char* path, const char* args, const char* workdir, g_security_level securityLevel,
                         g_pid* pid);
g_spawn_status g_spawn_po(const char* path, const char* args, const char* workdir, g_security_level securityLevel,
                          g_pid* pid, g_fd outStdio[3]);
g_spawn_status g_spawn_poi(const char* path, const char* args, const char* workdir, g_security_level securityLevel,
                           g_pid* pid, g_fd outStdio[3], g_fd inStdio[3]);
g_spawn_status g_spawn_poid(const char* path, const char* args, const char* workdir, g_security_level securityLevel,
                            g_pid* pid, g_fd outStdio[3], const g_fd inStdio[3],
                            g_spawn_validation_details* outValidationDetails);

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
 * Kills a process.
 *
 * @param pid
 * 		the process id
 *
 * @security-level APPLICATION
 */
g_kill_status g_kill(g_pid pid);

/**
 * @return a pointer to the user-thread object in the TLS of the current thread.
 */
g_user_threadlocal* g_task_get_tls();

/**
 * Returns a pointer to the process information structure. This structure for example
 * contains a list of all loaded objects.
 *
 * @return a pointer to a g_process_info
 */
g_process_info* g_process_get_info();

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
 * Sleeps for the given amount of milliseconds.
 *
 * @param ms
 * 		the milliseconds to sleep
 *
 * @security-level APPLICATION
 */
void g_sleep(uint64_t ms);


/**
 * Yields, causing a switch to the next process.
 *
 * @security-level APPLICATION
 */
void g_yield();

/**
 * Yields, trying to cooperatively yield to the given target task.
 *
 * @security-level APPLICATION
 */
void g_yield_t(g_tid target);

/**
 * @return local clock time in milliseconds
 *
 * @security-level APPLICATION
 */
uint64_t g_millis();

/**
 * @return elapsed time from HPET
 *
 * @security-level APPLICATION
 */
uint64_t g_nanos();

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
 * Write a task dump to the kernel log.
 *
 * @security-level APPLICATION
 */
void g_dump();

__END_C

#endif
