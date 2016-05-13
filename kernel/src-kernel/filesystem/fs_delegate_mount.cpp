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

#include "filesystem/fs_delegate_mount.hpp"
#include "filesystem/filesystem.hpp"
#include "utils/string.hpp"
#include "logger/logger.hpp"
#include "kernel.hpp"
#include "ghost/utils/local.hpp"

/**
 *
 */
g_fs_transaction_id g_fs_delegate_mount::request_discovery(g_thread* requester, g_fs_node* parent, char* child, g_fs_transaction_handler_discovery* handler) {

	// the ramdisk handler is doing it's work immediately and doesn't request another process
	g_fs_transaction_id id = g_fs_transaction_store::next_transaction();

	// root elements must exist and cannot be discovered
	handler->status = G_FS_DISCOVERY_NOT_FOUND;
	g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);

	return id;
}

/**
 *
 */
void g_fs_delegate_mount::finish_discovery(g_thread* requester, g_fs_transaction_handler_discovery* handler) {
	// nothing to do here
}

/**
 *
 */
g_fs_transaction_id g_fs_delegate_mount::request_read(g_thread* requester, g_fs_node* node, int64_t length, g_contextual<uint8_t*> buffer,
		g_file_descriptor_content* fd, g_fs_transaction_handler_read* handler) {

	// start/repeat transaction
	g_fs_transaction_id id;
	if (handler->wants_repeat_transaction()) {
		id = handler->get_repeated_transaction();
	} else {
		id = g_fs_transaction_store::next_transaction();
	}

	handler->status = G_FS_READ_ERROR;
	g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);

	return id;
}

/**
 *
 */
void g_fs_delegate_mount::finish_read(g_thread* requester, g_fs_read_status* out_status, int64_t* out_result, g_file_descriptor_content* fd) {
}

/**
 *
 */
g_fs_transaction_id g_fs_delegate_mount::request_write(g_thread* requester, g_fs_node* node, int64_t length, g_contextual<uint8_t*> buffer,
		g_file_descriptor_content* fd, g_fs_transaction_handler_write* handler) {

	// start/repeat transaction
	g_fs_transaction_id id;
	if (handler->wants_repeat_transaction()) {
		id = handler->get_repeated_transaction();
	} else {
		id = g_fs_transaction_store::next_transaction();
	}

	handler->status = G_FS_WRITE_ERROR;
	g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);

	return id;
}

/**
 *
 */
void g_fs_delegate_mount::finish_write(g_thread* requester, g_fs_write_status* out_status, int64_t* out_result, g_file_descriptor_content* fd) {
}

/**
 *
 */
g_fs_transaction_id g_fs_delegate_mount::request_get_length(g_thread* requester, g_fs_node* node, g_fs_transaction_handler_get_length* handler) {

	// the ramdisk handler is doing it's work immediately and doesn't request another process
	g_fs_transaction_id id = g_fs_transaction_store::next_transaction();

	handler->status = G_FS_LENGTH_ERROR;
	handler->length = -1;

	g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);
	return id;
}

/**
 *
 */
void g_fs_delegate_mount::finish_get_length(g_thread* requester, g_fs_transaction_handler_get_length* handler) {

}

/**
 *
 */
g_fs_transaction_id g_fs_delegate_mount::request_directory_refresh(g_thread* requester, g_fs_node* folder,
		g_fs_transaction_handler_directory_refresh* handler) {

	g_fs_transaction_id id = g_fs_transaction_store::next_transaction();

	// mount must not be refreshed, contents are always consistent
	folder->contents_valid = true;
	handler->status = G_FS_DIRECTORY_REFRESH_SUCCESSFUL;

	g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);
	return id;
}

/**
 *
 */
void g_fs_delegate_mount::finish_directory_refresh(g_thread* requester, g_fs_transaction_handler_directory_refresh* handler) {
}

/**
 *
 */
g_fs_transaction_id g_fs_delegate_mount::request_open(g_thread* requester, g_fs_node* node, char* filename, int32_t flags, int32_t mode,
		g_fs_transaction_handler_open* handler) {

	g_fs_transaction_id id = g_fs_transaction_store::next_transaction();

	// mount can't be opened
	g_log_warn("%! mountpoints can not be opened", "filesystem");
	handler->status = G_FS_OPEN_ERROR;

	g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);
	return id;
}

/**
 *
 */
void g_fs_delegate_mount::finish_open(g_thread* requester, g_fs_transaction_handler_open* handler) {
}

/**
 *
 */
g_fs_transaction_id g_fs_delegate_mount::request_close(g_thread* requester, g_fs_transaction_handler_close* handler, g_file_descriptor_content* fd,
		g_fs_node* node) {

	g_fs_transaction_id id = g_fs_transaction_store::next_transaction();

	// mount can't be opened
	g_log_warn("%! mountpoints can not be closed", "filesystem");
	handler->status = G_FS_CLOSE_ERROR;

	g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);
	return id;
}

/**
 *
 */
void g_fs_delegate_mount::finish_close(g_thread* requester, g_fs_transaction_handler_close* handler) {

}

