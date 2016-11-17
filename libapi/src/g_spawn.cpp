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
#include <ghost/utils/local.hpp>
#include "__internal.h"

// redirect
g_spawn_status g_spawn(const char* path, const char* args, const char* workdir, g_security_level securityLevel) {
	return g_spawn_poi(path, args, workdir, securityLevel, nullptr, nullptr, nullptr);
}

// redirect
g_spawn_status g_spawn_p(const char* path, const char* args, const char* workdir, g_security_level securityLevel, g_pid* pid) {
	return g_spawn_poi(path, args, workdir, securityLevel, pid, nullptr, nullptr);
}

// redirect
g_spawn_status g_spawn_po(const char* path, const char* args, const char* workdir, g_security_level securityLevel, g_pid* pid, g_fd out_stdio[3]) {
	return g_spawn_poi(path, args, workdir, securityLevel, pid, out_stdio, nullptr);
}

/**
 *
 */
g_spawn_status g_spawn_poi(const char* path, const char* args, const char* workdir, g_security_level securityLevel, g_pid* pid, g_fd out_stdio[3],
		g_fd in_stdio[3]) {

	g_spawn_status res = G_SPAWN_STATUS_UNKNOWN;
	uint32_t tid = g_get_tid();

	// get spawner task identifier
	g_tid spawner_tid = g_task_get_id(G_SPAWNER_IDENTIFIER);
	if (spawner_tid == -1) {
		return res;
	}

	// create transaction
	g_message_transaction tx = g_get_message_tx_id();

	// create request
	size_t path_bytes = __g_strlen(path) + 1;
	size_t args_bytes = __g_strlen(args) + 1;
	size_t workdir_bytes = __g_strlen(workdir) + 1;
	size_t requestlen = sizeof(g_spawn_command_spawn_request) + path_bytes + args_bytes + workdir_bytes;
	g_local<uint8_t> _request(new uint8_t[requestlen]);
	uint8_t* request = _request();

	// copy request contents
	g_spawn_command_spawn_request* req = (g_spawn_command_spawn_request*) request;
	req->header.command = G_SPAWN_COMMAND_SPAWN_REQUEST;
	req->security_level = securityLevel;
	req->path_bytes = path_bytes;
	req->args_bytes = args_bytes;
	req->workdir_bytes = workdir_bytes;

	if (in_stdio != nullptr) {
		req->stdin = in_stdio[0];
		req->stdout = in_stdio[1];
		req->stderr = in_stdio[2];
	} else {
		req->stdin = G_FD_NONE;
		req->stdout = G_FD_NONE;
		req->stderr = G_FD_NONE;
	}

	uint8_t* insert = request;
	insert += sizeof(g_spawn_command_spawn_request);
	__g_memcpy(insert, path, path_bytes);
	insert += path_bytes;
	__g_memcpy(insert, args, args_bytes);
	insert += args_bytes;
	__g_memcpy(insert, workdir, workdir_bytes);

	// send request to spawner
	g_send_message_t(spawner_tid, request, requestlen, tx);

	// receive response
	size_t resp_len = sizeof(g_message_header) + sizeof(g_spawn_command_spawn_response);
	g_local<uint8_t> resp_buf(new uint8_t[resp_len]);
	g_receive_message_t(resp_buf(), resp_len, tx);

	g_spawn_command_spawn_response* response = (g_spawn_command_spawn_response*) G_MESSAGE_CONTENT(resp_buf());

	// if successful, take response parameters
	if (response->status == G_SPAWN_STATUS_SUCCESSFUL) {

		if (pid != nullptr) {
			*pid = response->spawned_process_id;
		}

		if (out_stdio != nullptr) {
			out_stdio[0] = response->stdin_write;
			out_stdio[1] = response->stdout_read;
			out_stdio[2] = response->stderr_read;
		}
	}

	return response->status;
}
