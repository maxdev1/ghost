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
 * Pipes do not support discovery. We put a warning out though.
 */
g_fs_transaction_id g_fs_delegate_pipe::request_discovery(g_thread* requester, g_fs_node* parent, char* child, g_fs_transaction_handler_discovery* handler) {

	g_fs_transaction_id id = g_fs_transaction_store::next_transaction();
	handler->status = G_FS_DISCOVERY_ERROR;
	g_log_warn("%! no discovery support", "pipes");
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

	// find the right pipe
	g_pipe* pipe = g_pipes::get(node->phys_fs_id);
	if (pipe == nullptr) {

		// cancel with error if pipe not found
		handler->result = -1;
		handler->status = G_FS_READ_ERROR;
		g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);
		return id;
	}

	// find out what size can be read at maximum
	length = (pipe->size >= length) ? length : pipe->size;

	// calculate how much space remains until the end of the pipe
	uint32_t spaceToEnd = ((uint32_t) pipe->buffer + pipe->capacity) - (uint32_t) pipe->read;

	// if we want to read more than remaining
	if (length > spaceToEnd) {

		// copy bytes from the location of the read pointer
		g_memory::copy(buffer(), pipe->read, spaceToEnd);

		// copy remaining data
		uint32_t remaining = length - spaceToEnd;
		g_memory::copy(&buffer()[spaceToEnd], pipe->buffer, remaining);

		// set the read pointer to it's new location
		pipe->read = (uint8_t*) ((uint32_t) pipe->buffer + remaining);

	} else {
		// there are enough bytes left from read pointer, copy it to buffer
		g_memory::copy(buffer(), pipe->read, length);

		// set the read pointer to it's new location
		pipe->read = (uint8_t*) ((uint32_t) pipe->read + length);
	}

	// reset read pointer if end reached
	if (pipe->read == pipe->buffer + pipe->capacity) {
		pipe->read = pipe->buffer;
	}

	// if any bytes could be copied
	if (length > 0) {

		// decrease pipes remaining bytes
		pipe->size -= length;

		// finish with success
		handler->result = length;
		handler->status = G_FS_READ_SUCCESSFUL;
		g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);

		// TODO g_log_info("%! %i: read %i bytes -> size %i, read @%h, write @%h", "pipe", (int32_t ) node->phys_fs_id, (int32_t ) length, pipe->size, pipe->read - pipe->buffer, pipe->write - pipe->buffer);

		// handle blocking nodes
	} else if (node->is_blocking) {

		// avoid block if no one else has access to pipe
		if (!g_pipes::has_reference_from_other_process(pipe, requester->process->main->id)) {
			handler->result = 0;
			handler->status = G_FS_READ_ERROR;
			g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);
		} else {
			g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_REPEAT);
		}

	} else {
		// otherwise just finished with successful read status
		handler->result = 0;
		handler->status = G_FS_READ_SUCCESSFUL;
		g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);
	}

	return id;
}

/**
 *
 */
void g_fs_delegate_pipe::finish_read(g_thread* requester, g_fs_read_status* out_status, int64_t* out_result, g_file_descriptor_content* fd) {
	// nothing to do
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

	// find the pipe to write to
	g_pipe* pipe = g_pipes::get(node->phys_fs_id);
	if (pipe == nullptr) {
		handler->result = -1;
		handler->status = G_FS_WRITE_ERROR;
		g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);
		return id;
	}

	uint32_t space = (pipe->capacity - pipe->size);

	if (space > 0) {
		// check how many bytes can be written
		length = (space >= length) ? length : space;

		// check how many bytes can be written at the write pointer
		uint32_t spaceToEnd = ((uint32_t) pipe->buffer + pipe->capacity) - (uint32_t) pipe->write;

		if (length > spaceToEnd) {
			// write bytes at the write pointer
			g_memory::copy(pipe->write, buffer(), spaceToEnd);

			// write remaining bytes to the start of the pipe
			uint32_t remain = length - spaceToEnd;
			g_memory::copy(pipe->buffer, &buffer()[spaceToEnd], remain);

			// set the write pointer to the new location
			pipe->write = (uint8_t*) ((uint32_t) pipe->buffer + remain);

		} else {
			// just write bytes at write pointer
			g_memory::copy(pipe->write, buffer(), length);

			// set the write pointer to the new location
			pipe->write = (uint8_t*) ((uint32_t) pipe->write + length);
		}

		// reset write pointer if end reached
		if (pipe->write == pipe->buffer + pipe->capacity) {
			pipe->write = pipe->buffer;
		}

		pipe->size += length;
		handler->result = length;
		handler->status = G_FS_WRITE_SUCCESSFUL;
		g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);

		// TODO g_log_info("%! %i: wrote %i bytes -> size %i, read @%h, write @%h", "pipe", (int32_t ) node->phys_fs_id, (int32_t )length, pipe->size, pipe->read - pipe->buffer, pipe->write - pipe->buffer);

		// handle blocking
	} else if (node->is_blocking) {

		// try again later
		if (g_pipes::has_reference_from_other_process(pipe, requester->process->main->id)) {
			g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_REPEAT);
		} else {
			handler->result = -1;
			handler->status = G_FS_WRITE_ERROR;
			g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);
		}

	} else {
		handler->result = -1;
		handler->status = G_FS_WRITE_SUCCESSFUL;
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
	handler->status = G_FS_DIRECTORY_REFRESH_ERROR;

	g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);
	return id;
}

/**
 *
 */
void g_fs_delegate_pipe::finish_directory_refresh(g_thread* requester, g_fs_transaction_handler_directory_refresh* handler) {
}

/**
 *
 */
g_fs_transaction_id g_fs_delegate_pipe::request_open(g_thread* requester, g_fs_node* node, char* filename, int32_t flags, int32_t mode,
		g_fs_transaction_handler_open* handler) {

	g_fs_transaction_id id = g_fs_transaction_store::next_transaction();

	// pipes can never be opened
	g_log_warn("%! pipes can not be opened explicitly", "filesystem");
	handler->status = G_FS_OPEN_ERROR;

	g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);
	return id;
}

/**
 *
 */
void g_fs_delegate_pipe::finish_open(g_thread* requester, g_fs_transaction_handler_open* handler) {

}

/**
 *
 */
g_fs_transaction_id g_fs_delegate_pipe::request_close(g_thread* requester, g_fs_transaction_handler_close* handler, g_file_descriptor_content* fd,
		g_fs_node* node) {

	g_fs_transaction_id id = g_fs_transaction_store::next_transaction();
	// nothing to do here
	handler->status = G_FS_CLOSE_SUCCESSFUL;
	g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);
	return id;
}

/**
 *
 */
void g_fs_delegate_pipe::finish_close(g_thread* requester, g_fs_transaction_handler_close* handler) {

}
