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

#include "spawner.hpp"

#include <stdio.h>
#include <string.h>
#include <sstream>

#include <ghost/bytewise.h>
#include <ghostuser/utils/logger.hpp>
#include <ghostuser/io/files/file_utils.hpp>

#include "elf32_loader.hpp"

static g_pid pid;

/**
 *
 */
int main(int argc, char** argv) {
	receiveRequests();
}

/**
 *
 */
void receiveRequests() {

	// register this task as the system spawner
	if (!g_task_register_id(G_SPAWNER_IDENTIFIER)) {
		klog(
				"failed to initialize spawner service: could not register with identifier '%s'",
				G_SPAWNER_IDENTIFIER);
		return;
	}

	// service ready
	klog("spawner service ready");

	// obtain task id
	g_tid tid = g_get_tid();
	pid = g_get_pid();

	for (;;) {
		// receive incoming request
		g_message_empty(request);
		g_recv_msg(tid, &request);

		if (request.type == G_SPAWN_COMMAND_SPAWN) {
			processSpawnRequest (&request);

		} else {
			protocolError("received unknown command: code %i, task %i",
					request.type, request.sender);
		}
	}
}

/**
 *
 */
void protocolError(std::string msg, ...) {

	va_list va;
	va_start(va, msg);
	kvlog(("protocol error: " + msg).c_str(), va);
	va_end(va);
}

/**
 *
 */
void processSpawnRequest(g_message* request) {

	uint32_t tx = request->topic;
	g_fd req_pipe = request->parameterA;
	g_tid req_tid = request->sender;

	// clone pipe read end from requesting process
	g_fs_clonefd_status clone_s;
	g_fd pipe_r = g_clone_fd_s(req_pipe, g_get_pid_for_tid(req_tid), pid,
			&clone_s);
	if (clone_s != G_FS_CLONEFD_SUCCESSFUL) {
		return protocolError("unable to access incoming");
	}

	// read path and arguments
	std::string path;
	if (!g_file_utils::read_string(pipe_r, path)) {
		g_close(pipe_r);
		return protocolError("unable to read incoming path argument");
	}

	std::string args;
	if (!g_file_utils::read_string(pipe_r, args)) {
		g_close(pipe_r);
		return protocolError("unable to read incoming cli-arg argument");
	}

	// read security level
	uint8_t sec_lvl_b[4];
	if (!g_file_utils::read_bytes(pipe_r, sec_lvl_b, 4)) {
		g_close(pipe_r);
		return protocolError("unable to read incoming sec-lvl argument");
	}
	uint32_t sec_lvl;
	G_BW_GET_LE_4(sec_lvl_b, sec_lvl);

	// parameters ready, perform spawn
	g_pid o_pid;
	g_fd o_fd_inw;
	g_fd o_fd_outr;
	g_spawn_status spawn_status = spawn(path, args, sec_lvl, &o_pid, &o_fd_inw,
			&o_fd_outr);

	// send response
	g_message_empty(response);
	response.topic = tx;
	response.parameterA = spawn_status;
	response.parameterB = o_pid;
	response.parameterC = o_fd_inw;
	response.parameterD = o_fd_outr;
	g_send_msg(req_tid, &response);
}

/**
 *
 */
bool setup_stdio(g_pid pid, g_fd* out_inw, g_fd* out_outr) {

	// prepare stdio
	g_fd in[2];
	g_fd out[2];
	bool stdio = false;

	g_fs_pipe_status in_stat;
	g_pipe_s(&in[0], &in[1], &in_stat);
	g_fs_pipe_status out_stat;
	g_pipe_s(&out[0], &out[1], &out_stat);

	if (in_stat == G_FS_PIPE_SUCCESSFUL && out_stat == G_FS_PIPE_SUCCESSFUL) {

		uint32_t thispid = g_get_pid();

		// clone pipe fds into created process
		g_fs_clonefd_status target_stdin_stat;
		g_fd target_stdin = g_clone_fd_ts(in[1], thispid, STDIN_FILENO, pid,
				&target_stdin_stat);

		g_fd target_stdout = g_clone_fd_t(out[0], thispid, STDOUT_FILENO, pid);
		g_fd target_stderr = g_clone_fd_t(out[0], thispid, STDERR_FILENO, pid);

		// map & copy write end of stdin & read end of stdout to output
		*out_inw = g_clone_fd(in[0], thispid, pid);
		*out_outr = g_clone_fd(out[1], thispid, pid);

		// close pipe ends in this process
		g_close(in[0]);
		g_close(in[1]);
		g_close(out[0]);
		g_close(out[1]);

		if (target_stdin != -1 && target_stdout != -1 && target_stderr != -1) {
			stdio = true;
		}
	}

	return stdio;
}

/**
 *
 */
void write_cli_args(g_process_creation_identifier target_proc,
		std::string args) {

	// prepare well-sized buffer
	const char* args_cstr = args.c_str();
	char* args_buf = new char[G_CLIARGS_BUFFER_LENGTH];
	size_t args_len = strlen(args_cstr);

	// copy arguments to buffer
	memcpy(args_buf, args_cstr, args_len);
	args_buf[args_len] = 0;

	// store arguments
	g_cli_args_store(target_proc, args_buf);

	// free buffer
	delete args_buf;
}

/**
 *
 */
binary_format_t detect_format(g_fd file) {

	// try to detect the format
	binary_format_t form = BF_UNKNOWN;

	if (elf32_loader_t::check_for_elf_binary_and_reset(file) == LS_SUCCESSFUL) {
		form = BF_ELF32;
	}

	return form;
}

/**
 *
 */
g_spawn_status spawn(std::string path, std::string args,
		g_security_level sec_lvl, uint32_t* out_pid, int* out_fd_inw,
		int* out_fd_outr) {

	// open input file
	g_fd file = g_open(path.c_str());
	if (file == -1) {
		g_logger::log("unable to open file: \"" + path + "\"");
		return G_SPAWN_STATUS_IO_ERROR;
	}

	// detect format
	binary_format_t format = detect_format(file);
	if (format == BF_UNKNOWN) {
		g_logger::log("binary has unknown format: \"" + path + "\"");
		return G_SPAWN_STATUS_FORMAT_ERROR;
	}

	// create empty target process
	auto target_proc = g_create_empty_process(sec_lvl);
	g_pid pid = g_get_created_process_id(target_proc);

	std::stringstream info;
	info << "loading \"" << path << "\" to process " << pid;
	g_logger::log(info.str());

	// create a loader
	loader_t* loader;
	if (format == BF_ELF32) {
		loader = new elf32_loader_t(target_proc, file);
	} else {
		g_logger::log("no loader implementation for binary format %i", format);
		return G_SPAWN_STATUS_FORMAT_ERROR;
	}

	// setup standard I/O
	if (!setup_stdio(pid, out_fd_inw, out_fd_outr)) {
		klog("unable to setup stdio for process %i", pid);
	}

	// perform loading
	uintptr_t entry_address;
	loader_status_t ldr_stat = loader->load(&entry_address);
	g_spawn_status spawn_stat = G_SPAWN_STATUS_UNKNOWN;

	if (ldr_stat == LS_SUCCESSFUL) {
		// push command line arguments
		write_cli_args(target_proc, args);

		// attached loaded process
		g_attach_created_process(target_proc, entry_address);

		// out-set process id
		*out_pid = pid;
		spawn_stat = G_SPAWN_STATUS_SUCCESSFUL;

	} else {
		// cancel creation & let kernel clean up
		g_cancel_process_creation(target_proc);

		if (ldr_stat == LS_IO_ERROR) {
			spawn_stat = G_SPAWN_STATUS_IO_ERROR;

		} else if (ldr_stat == LS_FORMAT_ERROR) {
			spawn_stat = G_SPAWN_STATUS_FORMAT_ERROR;

		} else if (ldr_stat == LS_MEMORY_ERROR) {
			spawn_stat = G_SPAWN_STATUS_MEMORY_ERROR;

		} else {
			klog("loader return unknown status %i on failure", ldr_stat);
			spawn_stat = G_SPAWN_STATUS_UNKNOWN;
		}
	}

	// Close binary file
	g_close(file);

	return spawn_stat;
}

