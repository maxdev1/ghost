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

#include "filesystem/fs_delegate_pipe.hpp"
#include "filesystem/filesystem.hpp"
#include "utils/string.hpp"
#include "logger/logger.hpp"
#include "kernel.hpp"

/**
 *
 */
g_fs_transaction_id g_fs_delegate_pipe::request_discovery(g_thread* requester, g_fs_node* parent, char* child, g_fs_transaction_handler_discovery* handler) {

	g_fs_transaction_id id = g_fs_transaction_store::next_transaction();
	handler->status = G_FS_DISCOVERY_ERROR;
	g_log_info("discovery is not supported on pipes");
	g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);
	return id;
}

/**
 *
 */
void g_fs_delegate_pipe::finish_discovery(g_thread* requester, g_fs_transaction_handler_discovery* handler) {
	// nothing to do here
}

/**
 *
 */
g_fs_transaction_id g_fs_delegate_pipe::request_read(g_thread* requester, g_fs_node* node, int64_t length, g_contextual<uint8_t*> buffer,
		g_file_descriptor_content* fd, g_fs_transaction_handler_read* handler) {

	// start/repeat transaction
	g_fs_transaction_id id;
	if (handler->wants_repeat_transaction()) {
		id = handler->get_repeated_transaction();
	} else {
		id = g_fs_transaction_store::next_transaction();
	}

	g_pipe* pipe = g_pipes::get(node->phys_fs_id);
	if (pipe) {
		length = (pipe->size > length) ? length : pipe->size;

		uint32_t endspc = ((uint32_t) pipe->buffer + pipe->capacity) - (uint32_t) pipe->read;

		if (length > endspc) {
			uint32_t remain = length - endspc;
			g_memory::copy(buffer(), pipe->read, endspc);
			g_memory::copy(&buffer()[endspc], pipe->buffer, remain);
			pipe->read = (uint8_t*) ((uint32_t) pipe->buffer + remain);
		} else {
			g_memory::copy(buffer(), pipe->read, length);
			pipe->read = (uint8_t*) ((uint32_t) pipe->read + length);
		}

		if (length > 0) {
			pipe->size -= length;
			handler->result = length;
			handler->status = G_FS_READ_SUCCESSFUL;
			g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);
		} else {
			if (node->is_blocking) {
				// avoid block if no one else has access to pipe
				if (!g_pipes::has_reference_from_other_process(pipe, requester->process->main->id)) {
					handler->result = 0;
					handler->status = G_FS_READ_ERROR;
					g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);
				} else {
					g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_REPEAT);
				}
			} else {
				handler->result = 0;
				handler->status = G_FS_READ_SUCCESSFUL;
				g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);
			}
		}
	} else {
		handler->result = -1;
		handler->status = G_FS_READ_ERROR;
		g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);
	}

	return id;
}

/**
 *
 */
void g_fs_delegate_pipe::finish_read(g_thread* requester, g_fs_read_status* out_status, int64_t* out_result, g_file_descriptor_content* fd) {
}

/**
 *
 */
g_fs_transaction_id g_fs_delegate_pipe::request_write(g_thread* requester, g_fs_node* node, int64_t length, g_contextual<uint8_t*> buffer,
		g_file_descriptor_content* fd, g_fs_transaction_handler_write* handler) {

	// start/repeat transaction
	g_fs_transaction_id id;
	if (handler->wants_repeat_transaction()) {
		id = handler->get_repeated_transaction();
	} else {
		id = g_fs_transaction_store::next_transaction();
	}

	g_pipe* pipe = g_pipes::get(node->phys_fs_id);
	if (pipe) {
		int space = (pipe->capacity - pipe->size);

		if (space > 0) {
			length = (space > length) ? length : space;

			uint32_t endspc = ((uint32_t) pipe->buffer + pipe->capacity) - (uint32_t) pipe->write;

			if (length > endspc) {
				uint32_t remain = length - endspc;
				g_memory::copy(pipe->write, buffer(), endspc);
				g_memory::copy(pipe->buffer, &buffer()[endspc], remain);
				pipe->write = (uint8_t*) ((uint32_t) pipe->buffer + remain);
			} else {
				g_memory::copy(pipe->write, buffer(), length);
				pipe->write = (uint8_t*) ((uint32_t) pipe->write + length);
			}

			pipe->size += length;
			handler->result = length;
			handler->status = G_FS_WRITE_SUCCESSFUL;
			g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);
		} else {
			if (node->is_blocking) {
				// avoid block if no one else has access to pipe
				if (!g_pipes::has_reference_from_other_process(pipe, requester->process->main->id)) {
					handler->result = 0;
					handler->status = G_FS_WRITE_ERROR;
					g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);
				} else {
					g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_REPEAT);
				}
			} else {
				handler->result = 0;
				handler->status = G_FS_WRITE_SUCCESSFUL;
				g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);
			}
		}
	} else {
		handler->result = -1;
		handler->status = G_FS_WRITE_ERROR;
		g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);
	}

	return id;
}

/**
 *
 */
void g_fs_delegate_pipe::finish_write(g_thread* requester, g_fs_write_status* out_status, int64_t* out_result, g_file_descriptor_content* fd) {
}

/**
 *
 */
g_fs_transaction_id g_fs_delegate_pipe::request_get_length(g_thread* requester, g_fs_node* node, g_fs_transaction_handler_get_length* handler) {

	g_fs_transaction_id id = g_fs_transaction_store::next_transaction();

	g_pipe* pipe = g_pipes::get(node->phys_fs_id);
	if (pipe) {
		handler->length = pipe->capacity;
		handler->status = G_FS_LENGTH_SUCCESSFUL;
	} else {
		handler->length = -1;
		handler->status = G_FS_LENGTH_ERROR;
	}
	g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);

	return id;
}

/**
 *
 */
void g_fs_delegate_pipe::finish_get_length(g_thread* requester, g_fs_transaction_handler_get_length* handler) {
}

/**
 *
 */
g_fs_transaction_id g_fs_delegate_pipe::request_directory_refresh(g_thread* requester, g_fs_node* folder, g_fs_transaction_handler_directory_refresh* handler) {

	g_fs_transaction_id id = g_fs_transaction_store::next_transaction();

	// pipe has no children
	folder->contents_valid = true;
	handler->status = G_FS_DIRECTORY_REFRESH_SUCCESSFUL;

	g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);
	return id;
}

/**
 *
 */
void g_fs_delegate_pipe::finish_directory_refresh(g_thread* requester, g_fs_transaction_handler_directory_refresh* handler) {
}
