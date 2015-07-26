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
 */
void processSpawnRequest(g_message* request);

/**
 * Creates the standard input & output streams for the process
 * with the id <pid>. Writes the result to the output parameters
 *
 * @param pid
 * 		target process id
 * @param out_inw
 * 		write end of stdin of the process
 * @param out_outr
 * 		read end of stdout of the process
 */
bool setup_stdio(g_pid pid, g_fd* out_inw, g_fd* out_outr);

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
 * @param sec_lvl
 * 		security level
 */
g_spawn_status spawn(std::string path, std::string args,
		g_security_level sec_lvl, uint32_t* out_pid, int* out_fd_inw,
		int* out_fd_outr);

#endif
