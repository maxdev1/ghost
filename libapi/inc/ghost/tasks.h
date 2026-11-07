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
 * Terminates the calling process and all its threads without returning.
 *
 * This bypasses C library exit handlers and stdio cleanup; use exit() when those are required.
 *
 * @param[in] status Requested exit code; the current kernel does not retain it.
 * @security-level APPLICATION
 */
void g_exit(int status);

/**
 * Retrieves the calling process's identifier.
 *
 * @return The current process ID.
 * @security-level APPLICATION
 */
g_pid g_get_pid();

/**
 * Requests a process's parent ID through a currently unimplemented kernel call.
 *
 * The current implementation only logs that the operation is unimplemented.
 *
 * @param[in] pid Process ID whose parent would be queried.
 * @return Unspecified in the current kernel; do not use this value.
 * @security-level APPLICATION
 */
g_pid g_get_parent_pid(g_pid pid);

/**
 * Retrieves the calling thread's identifier.
 *
 * @return The current thread ID; the main thread's ID equals its process ID.
 * @security-level APPLICATION
 */
g_tid g_get_tid();

/**
 * Finds the process containing a thread.
 *
 * @param[in] tid Thread ID to look up.
 * @return The owning process ID, or G_PID_NONE if the thread does not exist.
 * @security-level APPLICATION
 */
g_pid g_get_pid_for_tid(g_tid tid);

/**
 * Creates a thread in the calling process with no user data or CPU affinity.
 *
 * Equivalent to g_create_task_das(function, 0, G_TASK_CORE_AFFINITY_NONE, 0).
 *
 * @param[in] function Thread entry with signature void(void*), cast to void*. Returning from it
 *                     exits the thread.
 * @return The new thread ID, or G_TID_NONE on failure.
 * @security-level APPLICATION
 */
g_tid g_create_task(void* function);

/**
 * Creates a thread with user data and no CPU affinity.
 *
 * Equivalent to g_create_task_das(function, userData, G_TASK_CORE_AFFINITY_NONE, 0).
 *
 * @param[in] function Thread entry with signature void(void*), cast to void*. Returning from it
 *                     exits the thread.
 * @param[in] userData Opaque pointer passed unchanged to the entry; may be null. Keep referenced
 *                     data alive until the thread finishes.
 * @return The new thread ID, or G_TID_NONE on failure.
 * @security-level APPLICATION
 */
g_tid g_create_task_d(void* function, void* userData);

/**
 * Creates a thread with a CPU affinity and no user data.
 *
 * Equivalent to g_create_task_das(function, 0, coreAffinity, 0).
 *
 * @param[in] function Thread entry with signature void(void*), cast to void*. Returning from it
 *                     exits the thread.
 * @param[in] coreAffinity Logical processor index, or G_TASK_CORE_AFFINITY_NONE for scheduler
 *                         placement.
 * @return The new thread ID, or G_TID_NONE on failure.
 * @security-level APPLICATION
 */
g_tid g_create_task_a(void* function, uint8_t coreAffinity);

/**
 * Creates a thread with user data and optional status output.
 *
 * Equivalent to g_create_task_das(function, userData, G_TASK_CORE_AFFINITY_NONE, outStatus).
 *
 * @param[in] function Thread entry with signature void(void*), cast to void*. Returning from it
 *                     exits the thread.
 * @param[in] userData Opaque pointer passed unchanged to the entry; may be null. Keep referenced
 *                     data alive until the thread finishes.
 * @param[out] outStatus Receives a g_create_task_status code; may be null.
 * @return The new thread ID, or G_TID_NONE on failure.
 * @security-level APPLICATION
 */
g_tid g_create_task_ds(void* function, void* userData, g_create_task_status* outStatus);

/**
 * Creates a thread with user data and a CPU affinity.
 *
 * Equivalent to g_create_task_das(function, userData, coreAffinity, 0).
 *
 * @param[in] function Thread entry with signature void(void*), cast to void*. Returning from it
 *                     exits the thread.
 * @param[in] userData Opaque pointer passed unchanged to the entry; may be null. Keep referenced
 *                     data alive until the thread finishes.
 * @param[in] coreAffinity Logical processor index, or G_TASK_CORE_AFFINITY_NONE for scheduler
 *                         placement.
 * @return The new thread ID, or G_TID_NONE on failure.
 * @security-level APPLICATION
 */
g_tid g_create_task_da(void* function, void* userData, uint8_t coreAffinity);

/**
 * Creates a thread with user data, CPU placement and optional status output.
 *
 * The thread shares the process address space and inherits its main thread's security level. An
 * invalid CPU index fails. The thread may begin executing before this call returns.
 *
 * @param[in] function Thread entry with signature void(void*), cast to void*. Returning from it
 *                     exits the thread.
 * @param[in] userData Opaque pointer passed unchanged to the entry; may be null. Keep referenced
 *                     data alive until the thread finishes.
 * @param[in] coreAffinity Logical processor index, or G_TASK_CORE_AFFINITY_NONE for scheduler
 *                         placement.
 * @param[out] outStatus Receives a g_create_task_status code; may be null.
 * @return The new thread ID, or G_TID_NONE on failure.
 * @security-level APPLICATION
 */
g_tid g_create_task_das(void* function, void* userData, uint8_t coreAffinity, g_create_task_status* outStatus);

/**
 * Terminates only the calling thread without returning.
 *
 * Does not run C library cleanup. Use g_exit() to terminate the entire process.
 *
 * @security-level APPLICATION
 */
void g_exit_task();


/**
 * Registers the calling thread under a service name.
 *
 * A stronger-privilege caller may replace a weaker registration. Other tasks can discover the
 * resulting thread ID with g_task_get_by_name() or g_task_await_by_name().
 *
 * @param[in] name Non-null, null-terminated registration name.
 * @return Nonzero on success, or zero if the name is already held at equal or stronger privilege.
 * @security-level APPLICATION
 */
g_bool g_task_register_name(const char* name);

/**
 * Waits until a service name resolves to a registered thread.
 *
 * The returned thread may exit after lookup; this call does not retain it.
 *
 * @param[in] name Non-null, null-terminated service name.
 * @return The registered thread ID.
 * @security-level APPLICATION
 */
g_tid g_task_await_by_name(const char* name);

/**
 * Looks up a registered service name without waiting.
 *
 * @param[in] name Non-null, null-terminated service name.
 * @return The registered thread ID, or G_TID_NONE if the name is absent.
 * @security-level APPLICATION
 */
g_tid g_task_get_by_name(const char* name);

/**
 * Retrieves the launch context recorded for the calling process.
 *
 * Boot processes normally have UNKNOWN context. Launch context is metadata and does not grant
 * privileges.
 *
 * @return G_LAUNCH_MODE_UNKNOWN, G_LAUNCH_MODE_UI or G_LAUNCH_MODE_TERMINAL.
 * @security-level APPLICATION
 */
g_launch_mode g_get_launch_mode();

/**
 * Starts an executable with inherited launch context and no standard streams.
 *
 * Equivalent to g_spawn_poid(path, args, workdir, securityLevel, 0, 0, 0, 0).
 *
 * @param[in] path Non-null, null-terminated executable path; relative paths use the caller's
 *                 working directory.
 * @param[in] args Raw command-line string to copy into the child, or null for no arguments.
 * @param[in] workdir Initial child working-directory string, or null for the default root
 *                    directory.
 * @param[in] securityLevel Requested G_SECURITY_LEVEL_* value; cannot be more privileged than the
 *                          caller.
 * @return A g_spawn_status code; G_SPAWN_STATUS_SUCCESSFUL means the child was started.
 * @security-level APPLICATION
 */
g_spawn_status g_spawn(const char* path, const char* args, const char* workdir, g_security_level securityLevel);

/**
 * Starts an executable and optionally returns its process ID.
 *
 * Launch context is inherited. No standard streams are supplied; use a variant with inStdio to
 * clone them.
 *
 * @param[in] path Non-null, null-terminated executable path; relative paths use the caller's
 *                 working directory.
 * @param[in] args Raw command-line string to copy into the child, or null for no arguments.
 * @param[in] workdir Initial child working-directory string, or null for the default root
 *                    directory.
 * @param[in] securityLevel Requested G_SECURITY_LEVEL_* value; cannot be more privileged than the
 *                          caller.
 * @param[out] outPid Receives the child process ID on success; may be null.
 * @return A g_spawn_status code; G_SPAWN_STATUS_SUCCESSFUL means the child was started.
 * @security-level APPLICATION
 */
g_spawn_status g_spawn_p(const char* path, const char* args, const char* workdir, g_security_level securityLevel,
                         g_pid* outPid);

/**
 * Starts an executable and reports its process ID and standard-stream descriptors.
 *
 * Launch context is inherited. This variant supplies no input descriptors, so the child's standard
 * streams remain unopened; it does not create pipes.
 *
 * @param[in] path Non-null, null-terminated executable path; relative paths use the caller's
 *                 working directory.
 * @param[in] args Raw command-line string to copy into the child, or null for no arguments.
 * @param[in] workdir Initial child working-directory string, or null for the default root
 *                    directory.
 * @param[in] securityLevel Requested G_SECURITY_LEVEL_* value; cannot be more privileged than the
 *                          caller.
 * @param[out] outPid Receives the child process ID on success; may be null.
 * @param[out] outStdio Optional three-element output for child stdin/stdout/stderr descriptor
 *                      numbers. These belong to the child, not the caller; inspect only after
 *                      success.
 * @return A g_spawn_status code; G_SPAWN_STATUS_SUCCESSFUL means the child was started.
 * @security-level APPLICATION
 */
g_spawn_status g_spawn_po(const char* path, const char* args, const char* workdir, g_security_level securityLevel,
                          g_pid* outPid, g_fd outStdio[3]);

/**
 * Starts an executable with optional standard streams cloned from the caller.
 *
 * Equivalent to g_spawn_poid(path, args, workdir, securityLevel, outPid, outStdio, inStdio, 0).
 * Input descriptors are not modified or closed; no pipes are created automatically.
 *
 * @param[in] path Non-null, null-terminated executable path; relative paths use the caller's
 *                 working directory.
 * @param[in] args Raw command-line string to copy into the child, or null for no arguments.
 * @param[in] workdir Initial child working-directory string, or null for the default root
 *                    directory.
 * @param[in] securityLevel Requested G_SECURITY_LEVEL_* value; cannot be more privileged than the
 *                          caller.
 * @param[out] outPid Receives the child process ID on success; may be null.
 * @param[out] outStdio Optional three-element output for child stdin/stdout/stderr descriptor
 *                      numbers. These belong to the child, not the caller; inspect only after
 *                      success.
 * @param[in] inStdio Three caller descriptors to clone as child stdin/stdout/stderr. Null or
 *                    G_FD_NONE entries leave those child streams unopened.
 * @return A g_spawn_status code; G_SPAWN_STATUS_SUCCESSFUL means the child was started.
 * @security-level APPLICATION
 */
g_spawn_status g_spawn_poi(const char* path, const char* args, const char* workdir, g_security_level securityLevel,
                           g_pid* outPid, g_fd outStdio[3], g_fd inStdio[3]);

/**
 * Starts an executable with standard-stream inputs and validation diagnostics.
 *
 * Equivalent to g_spawn_poidm() with G_LAUNCH_MODE_INHERIT. The caller retains its input
 * descriptors; the child receives cloned descriptors, not newly created pipes.
 *
 * @param[in] path Non-null, null-terminated executable path; relative paths use the caller's
 *                 working directory.
 * @param[in] args Raw command-line string to copy into the child, or null for no arguments.
 * @param[in] workdir Initial child working-directory string, or null for the default root
 *                    directory.
 * @param[in] securityLevel Requested G_SECURITY_LEVEL_* value; cannot be more privileged than the
 *                          caller.
 * @param[out] outPid Receives the child process ID on success; may be null.
 * @param[out] outStdio Optional three-element output for child stdin/stdout/stderr descriptor
 *                      numbers. These belong to the child, not the caller; inspect only after
 *                      success.
 * @param[in] inStdio Three caller descriptors to clone as child stdin/stdout/stderr. Null or
 *                    G_FD_NONE entries leave those child streams unopened.
 * @param[out] outValidationDetails Optional output for g_spawn_validation_details, meaningful when
 *                                  executable validation was performed.
 * @return A g_spawn_status code; G_SPAWN_STATUS_SUCCESSFUL means the child was started.
 * @security-level APPLICATION
 */
g_spawn_status g_spawn_poid(const char* path, const char* args, const char* workdir, g_security_level securityLevel,
                            g_pid* outPid, g_fd outStdio[3], const g_fd inStdio[3],
                            g_spawn_validation_details* outValidationDetails);

/**
 * Starts an executable with an explicit launch context.
 *
 * Equivalent to g_spawn_poidm() without validation-detail output. The caller retains its input
 * descriptors; launch context does not alter securityLevel.
 *
 * @param[in] path Non-null, null-terminated executable path; relative paths use the caller's
 *                 working directory.
 * @param[in] args Raw command-line string to copy into the child, or null for no arguments.
 * @param[in] workdir Initial child working-directory string, or null for the default root
 *                    directory.
 * @param[in] securityLevel Requested G_SECURITY_LEVEL_* value; cannot be more privileged than the
 *                          caller.
 * @param[out] outPid Receives the child process ID on success; may be null.
 * @param[out] outStdio Optional three-element output for child stdin/stdout/stderr descriptor
 *                      numbers. These belong to the child, not the caller; inspect only after
 *                      success.
 * @param[in] inStdio Three caller descriptors to clone as child stdin/stdout/stderr. Null or
 *                    G_FD_NONE entries leave those child streams unopened.
 * @param[in] mode G_LAUNCH_MODE_UNKNOWN, UI or TERMINAL, or G_LAUNCH_MODE_INHERIT to retain the
 *                 parent's launch context.
 * @return A g_spawn_status code; G_SPAWN_STATUS_SUCCESSFUL means the child was started.
 * @security-level APPLICATION
 */
g_spawn_status g_spawn_poim(const char* path, const char* args, const char* workdir,
                           g_security_level securityLevel, g_pid* outPid, g_fd outStdio[3],
                           const g_fd inStdio[3], g_launch_mode mode);

/**
 * Starts an executable with stream cloning, diagnostics and launch context.
 *
 * The executable and requested privilege are validated before the child runs. Supplied streams are
 * cloned into child descriptors 0, 1 and 2; omitted streams stay unopened. No pipes are created.
 * The caller retains its input descriptors. The workdir string is stored without checking that the
 * directory exists.
 *
 * @param[in] path Non-null, null-terminated executable path; relative paths use the caller's
 *                 working directory.
 * @param[in] args Raw command-line string to copy into the child, or null for no arguments.
 * @param[in] workdir Initial child working-directory string, or null for the default root
 *                    directory.
 * @param[in] securityLevel Requested G_SECURITY_LEVEL_* value; cannot be more privileged than the
 *                          caller.
 * @param[out] outPid Receives the child process ID on success; may be null.
 * @param[out] outStdio Optional three-element output for child stdin/stdout/stderr descriptor
 *                      numbers. These belong to the child, not the caller; inspect only after
 *                      success.
 * @param[in] inStdio Three caller descriptors to clone as child stdin/stdout/stderr. Null or
 *                    G_FD_NONE entries leave those child streams unopened.
 * @param[out] outValidationDetails Optional output for g_spawn_validation_details, meaningful when
 *                                  executable validation was performed.
 * @param[in] mode G_LAUNCH_MODE_UNKNOWN, UI or TERMINAL, or G_LAUNCH_MODE_INHERIT to retain the
 *                 parent's launch context.
 * @return A g_spawn_status code; G_SPAWN_STATUS_SUCCESSFUL means the child was started.
 * @security-level APPLICATION
 */
g_spawn_status g_spawn_poidm(const char* path, const char* args, const char* workdir,
                            g_security_level securityLevel, g_pid* outPid, g_fd outStdio[3],
                            const g_fd inStdio[3], g_spawn_validation_details* outValidationDetails,
                            g_launch_mode mode);

/**
 * Copies the calling process's raw command-line arguments into a buffer.
 *
 * Writes an empty string when no arguments were supplied. Despite the historical name, the
 * arguments are not released or consumed; repeated calls copy them again.
 *
 * @param[out] buffer Non-null destination large enough for the entire argument string and its
 *                    terminator; no capacity is passed.
 * @security-level APPLICATION
 */
void g_cli_args_release(char* buffer);

/**
 * Requests termination of a process and all its threads.
 *
 * The caller cannot terminate a target with a stronger security level. Success requests
 * termination; it does not wait for all cleanup. Use g_join() to wait for task exit.
 *
 * @param[in] pid Process ID to terminate.
 * @return A g_kill_status code.
 * @security-level APPLICATION
 */
g_kill_status g_kill(g_pid pid);

/**
 * Retrieves the calling thread's user-space TLS control structure.
 *
 * @return A thread-owned g_user_threadlocal pointer; do not free it or retain it past thread exit.
 * @security-level APPLICATION
 */
g_user_threadlocal* g_task_get_tls();

/**
 * Retrieves the calling process's loader information.
 *
 * Also updates g_current_process_info. The kernel and loader own the structure and its nested
 * arrays.
 *
 * @return A process-owned g_process_info pointer containing loaded-object information; do not free
 *         it.
 * @security-level APPLICATION
 */
g_process_info* g_process_get_info();

/**
 * Requests process duplication through a currently unimplemented kernel call.
 *
 * The current implementation only logs that the operation is unimplemented.
 *
 * @return Unspecified in the current kernel; no child is created and the value must not be used.
 * @security-level APPLICATION
 */
g_tid g_fork();

/**
 * Waits until a thread has exited.
 *
 * Does not return an exit code and does not itself release user-managed thread resources.
 *
 * @param[in] tid Thread ID to wait for; an already absent thread returns immediately.
 * @security-level APPLICATION
 */
void g_join(g_tid tid);

/**
 * Suspends the calling thread for at least the requested kernel-clock interval.
 *
 * The thread becomes runnable when the interval expires; actual resumption also depends on
 * scheduling.
 *
 * @param[in] ms Sleep interval in milliseconds.
 * @security-level APPLICATION
 */
void g_sleep(uint64_t ms);


/**
 * Gives the scheduler an opportunity to run another thread.
 *
 * Equivalent to g_yield_t(G_TID_NONE).
 *
 * @security-level APPLICATION
 */
void g_yield();

/**
 * Yields execution while asking the scheduler to prefer a particular thread.
 *
 * The preference is best effort and does not guarantee that target runs next.
 *
 * @param[in] target Preferred thread ID, or G_TID_NONE for ordinary scheduling.
 * @security-level APPLICATION
 */
void g_yield_t(g_tid target);

/**
 * Reads elapsed time from the current core's kernel clock.
 *
 * @return The kernel clock value in milliseconds, not a calendar timestamp.
 * @security-level APPLICATION
 */
uint64_t g_millis();

/**
 * Reads elapsed kernel timer time in nanoseconds.
 *
 * Without HPET, resolution is limited to kernel clock ticks despite the nanosecond unit.
 *
 * @return Elapsed nanoseconds from HPET, or the kernel millisecond clock scaled to nanoseconds.
 * @security-level APPLICATION
 */
uint64_t g_nanos();

/**
 * Reads the current Unix time in seconds. Returns true on success.
 * Sets the output to -1 if the system clock is unavailable. A null output
 * pointer returns false.
 * @security-level APPLICATION
 */
g_bool g_time(int64_t* unixSeconds);

/**
 * Changes the calling process's working directory.
 *
 * The kernel verifies that the path names a directory and stores its absolute path. Relative input
 * is currently resolved from root, not from the previous working directory.
 *
 * @param[in] path Non-null, null-terminated directory path, resolved from the filesystem root.
 * @return A g_set_working_directory_status code; failure leaves the working directory unchanged.
 * @security-level APPLICATION
 */
g_set_working_directory_status g_set_working_directory(const char* path);

/**
 * Copies the calling process's working directory into a buffer.
 *
 * Equivalent to g_get_working_directory_l(buffer, G_PATH_MAX).
 *
 * @param[out] buffer Non-null destination with at least G_PATH_MAX bytes.
 * @return A g_get_working_directory_status code; inspect buffer only on success.
 * @security-level APPLICATION
 */
g_get_working_directory_status g_get_working_directory(char* buffer);

/**
 * Retrieves the working directory with an explicit destination capacity.
 *
 * Writes a null-terminated path on success, using root when no working directory is recorded. An
 * undersized buffer is left unchanged.
 *
 * @param[out] buffer Non-null destination with at least maxlen bytes.
 * @param[in] maxlen Destination capacity in bytes, including the terminator.
 * @return G_GET_WORKING_DIRECTORY_SUCCESSFUL, or G_GET_WORKING_DIRECTORY_SIZE_EXCEEDED if it does
 *         not fit.
 * @security-level APPLICATION
 */
g_get_working_directory_status g_get_working_directory_l(char* buffer, size_t maxlen);

/**
 * Copies the executable path recorded when the process was started.
 *
 * Writes an empty string if no executable path is recorded, as for boot tasks. This is the
 * executable path, not its parent directory.
 *
 * @param[out] buffer Non-null destination large enough for the complete path and terminator; no
 *                    capacity is passed.
 * @security-level APPLICATION
 */
void g_get_executable_path(char* buffer);

/**
 * Writes a scheduler task dump to the kernel log.
 *
 * @security-level APPLICATION
 */
void g_dump();

__END_C

#endif
