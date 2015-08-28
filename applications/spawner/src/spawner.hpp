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

#include <ghost.h>
#include <string>

#ifndef __SPAWNER__
#define __SPAWNER__

/**
 *
 */
enum binary_format_t {
	BF_UNKNOWN, BF_ELF32
};

/**
 * Receives incoming message requests.
 */
void receiveRequests();

/**
 *
 */
void protocolError(std::string msg, ...);

/**
 * Reads the arguments from the given pipe.
 *
 * @param request
 * 		incoming message
 * @param requester
 * 		tid of requester
 * @param tx
 * 		transaction to respond on
 */
void processSpawnRequest(g_spawn_command_spawn_request* request,
		g_tid requester, g_message_transaction tx);

/**
 * Creates the standard input & output streams for the process
 * with the id <pid>. Writes the result to the output parameters
 *
 * @param created_pid
 * 		target process id
 * @param requester_pid
 * 		process id of the requesting task
 * @param out_inw
 * 		write end of stdin of the process
 * @param out_outr
 * 		read end of stdout of the process
 * @param out_errr
 * 		read end of stderr of the process
 */
bool setup_stdio(g_pid created_pid, g_pid requester_pid, g_fd* out_inw,
		g_fd* out_outr, g_fd* out_errr, g_fd in_stdin, g_fd in_stdout,
		g_fd in_stderr);

/**
 * Places the given command line arguments in the kernel buffer
 * for the target process <target_proc>.
 *
 * @param target_proc
 * 		target process identifier
 * @param args
 * 		arguments to pass
 */
void write_cli_args(g_process_creation_identifier target_proc,
		std::string args);

/**
 *
 */
binary_format_t detect_format(g_fd file);

/**
 * Spawns the binary at <path> passing the arguments <args>. As security level,
 * the level <sec_lvl> is set. The out parameters are filled with the respective
 * values on success.
 *
 * @param path
 * 		binary path
 * @param args
 * 		command line arguments
 * @param workdir
 * 		target working directory
 * @param sec_lvl
 * 		security level
 * @param requester_pid
 * 		process id of the requester
 */
g_spawn_status spawn(const char* path, const char* args, const char* workdir,
		g_security_level sec_lvl, g_pid requester_pid, g_pid* out_pid,
		g_fd* out_stdin, g_fd* out_stdout, g_fd* out_stderr, g_fd in_stdin,
		g_fd in_stdout, g_fd in_stderr);

#endif
