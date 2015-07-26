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

#include "ghost.h"
#include "ghost/bytewise.h"
#include <string.h>

/**
 *
 */
g_spawn_status g_spawn(const char* path, const char* args, g_security_level securityLevel, uint32_t* pid, int stdio[2]) {

	g_spawn_status res = G_SPAWN_STATUS_UNKNOWN;
	uint32_t tid = g_get_tid();

	// get spawner task identifier
	g_tid spawner_tid = g_task_get_id(G_SPAWNER_IDENTIFIER);
	if (spawner_tid == -1) {
		return res;
	}

	// open streams
	g_fd pip_w;
	g_fd pip_r;
	g_fs_pipe_status s;
	g_pipe_s(&pip_w, &pip_r, &s);

	// pipe was opened?
	if (s == G_FS_PIPE_SUCCESSFUL) {

		// create transaction
		uint32_t tx = g_ipc_next_topic();

		// prepare spawn request
		g_message_empty(request);
		request.type = G_SPAWN_COMMAND_SPAWN;
		request.topic = tx;
		request.parameterA = pip_r;

		// send request to spawner
		g_send_msg(spawner_tid, &request);

		// write arguments to stream
		g_write(pip_w, path, strlen(path) + 1);
		g_write(pip_w, args, strlen(args) + 1);

		uint8_t sec_lvl_bytes[4];
		G_BW_PUT_LE_4(sec_lvl_bytes, securityLevel);
		g_write(pip_w, sec_lvl_bytes, 4);

		// receive response
		g_message_empty(response);
		g_recv_topic_msg(tid, tx, &response);

		// response contains status code
		res = (g_spawn_status) response.parameterA;

		// if successful, take response parameters
		if (res == G_SPAWN_STATUS_SUCCESSFUL) {
			*pid = response.parameterB;
			stdio[0] = response.parameterC;
			stdio[1] = response.parameterD;
		}
	}

	// close pipe
	g_close(pip_w);
	g_close(pip_r);

	return res;
}
