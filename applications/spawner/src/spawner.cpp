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

	size_t request_len_max = sizeof(g_message_header)
			+ sizeof(g_spawn_command_spawn_request) + 1024;
	uint8_t* request_buffer = new uint8_t[request_len_max];
	for (;;) {
		// receive incoming request
		g_message_receive_status stat = g_receive_message(request_buffer,
				request_len_max);

		if (stat != G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL) {
			protocolError("receiving command failed with code %i", stat);
			continue;
		}

		g_message_header* header = (g_message_header*) request_buffer;
		g_spawn_command_header* command_header =
				(g_spawn_command_header*) G_MESSAGE_CONTENT(header);
		if (command_header->command == G_SPAWN_COMMAND_SPAWN_REQUEST) {
			processSpawnRequest((g_spawn_command_spawn_request*) command_header,
					header->sender, header->transaction);

		} else {
			protocolError("received unknown command: code %i, task %i",
					command_header->command, header->sender);
		}
	}
	delete request_buffer;
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
void processSpawnRequest(g_spawn_command_spawn_request* request,
		g_tid requester, g_message_transaction tx) {

	g_security_level sec_lvl = request->security_level;
	const char* pos = (const char*) request;
	pos += sizeof(g_spawn_command_spawn_request);
	const char* path = pos;
	pos += request->path_bytes;
	const char* args = pos;
	pos += request->args_bytes;
	const char* workdir = pos;

	// parameters ready, perform spawn
	g_pid o_pid;
	g_fd o_fd_inw;
	g_fd o_fd_outr;
	g_fd o_fd_errr;
	g_pid requester_pid = g_get_pid_for_tid(requester);
	g_spawn_status spawn_status = spawn(path, args, workdir, sec_lvl,
			requester_pid, &o_pid, &o_fd_inw, &o_fd_outr, &o_fd_errr,
			request->stdin, request->stdout, request->stderr);

	// send response
	g_spawn_command_spawn_response response;
	response.spawned_process_id = o_pid;
	response.status = spawn_status;
	response.stdin_write = o_fd_inw;
	response.stdout_read = o_fd_outr;
	response.stderr_read = o_fd_errr;
	g_send_message_t(requester, &response,
			sizeof(g_spawn_command_spawn_response), tx);
}

/**
 *
 */
bool create_pipe(g_pid this_pid, g_pid requester_pid, g_pid target_pid,
		g_fd source, g_fd* out, g_fd target) {

	g_fd created = G_FD_NONE;
	if (source == G_FD_NONE) {

		// create pipe
		g_fd pipe[2];
		g_fs_pipe_status pipe_stat;
		g_pipe_s(&pipe[0], &pipe[1], &pipe_stat);

		if (pipe_stat == G_FS_PIPE_SUCCESSFUL) {
			// map into target & requester
			created = g_clone_fd_t(pipe[1], this_pid, target, target_pid);
			*out = g_clone_fd(pipe[0], this_pid, requester_pid);

			// close pipe here
			g_close(pipe[0]);
			g_close(pipe[1]);
		}

	} else {
		// map into target
		created = g_clone_fd_t(source, requester_pid, target, target_pid);
		*out = source;
	}

	return created != G_FD_NONE;
}

/**
 *
 */
bool setup_stdio(g_pid created_pid, g_pid requester_pid, g_fd* out_stdin,
		g_fd* out_stdout, g_fd* out_stderr, g_fd in_stdin, g_fd in_stdout,
		g_fd in_stderr) {

	uint32_t this_pid = g_get_pid();

	if (!create_pipe(this_pid, requester_pid, created_pid, in_stdin, out_stdin,
	STDIN_FILENO)) {
		return false;
	}

	if (!create_pipe(this_pid, requester_pid, created_pid, in_stdout,
			out_stdout,
			STDOUT_FILENO)) {
		return false;
	}

	if (!create_pipe(this_pid, requester_pid, created_pid, in_stderr,
			out_stderr,
			STDERR_FILENO)) {
		return false;
	}

	return true;
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
g_spawn_status spawn(const char* path, const char* args, const char* workdir,
		g_security_level sec_lvl, g_pid requester_pid, g_pid* out_pid,
		g_fd* out_stdin, g_fd* out_stdout, g_fd* out_stderr, g_fd in_stdin,
		g_fd in_stdout, g_fd in_stderr) {

	// open input file
	g_fd file = g_open(path);
	if (file == -1) {
		g_logger::log("unable to open file: \"%s\"", path);
		return G_SPAWN_STATUS_IO_ERROR;
	}

	// detect format
	binary_format_t format = detect_format(file);
	if (format == BF_UNKNOWN) {
		g_logger::log("binary has unknown format: \"%s\"", path);
		return G_SPAWN_STATUS_FORMAT_ERROR;
	}

	// create empty target process
	auto target_proc = g_create_empty_process(sec_lvl);
	g_pid target_pid = g_get_created_process_id(target_proc);

	// apply configuration
	g_process_configuration configuration;
	configuration.source_path = (char*) path;
	g_configure_process(target_proc, configuration);

	// create a loader
	loader_t* loader;
	if (format == BF_ELF32) {
		loader = new elf32_loader_t(target_proc, file);
	} else {
		g_logger::log("no loader implementation for binary format %i", format);
		return G_SPAWN_STATUS_FORMAT_ERROR;
	}

	// setup standard I/O
	if (!setup_stdio(target_pid, requester_pid, out_stdin, out_stdout,
			out_stderr, in_stdin, in_stdout, in_stderr)) {
		klog("unable to setup stdio for process %i", target_pid);
	}

	// perform loading
	uintptr_t entry_address;
	loader_status_t ldr_stat = loader->load(&entry_address);
	g_spawn_status spawn_stat = G_SPAWN_STATUS_UNKNOWN;

	if (ldr_stat == LS_SUCCESSFUL) {
		// push command line arguments
		write_cli_args(target_proc, args);

		// set working directory
		g_set_working_directory_p(workdir, target_proc);

		// attached loaded process
		g_attach_created_process(target_proc, entry_address);

		// out-set process id
		*out_pid = target_pid;
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

