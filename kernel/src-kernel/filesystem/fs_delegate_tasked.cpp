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

#include "filesystem/fs_delegate_tasked.hpp"
#include "filesystem/filesystem.hpp"
#include "utils/string.hpp"
#include "logger/logger.hpp"
#include "kernel.hpp"
#include "tasking/communication/message_controller.hpp"
#include "memory/address_space.hpp"
#include "memory/physical/pp_allocator.hpp"
#include "ghost/utils/local.hpp"

/**
 *
 */
g_fs_delegate_tasked::g_fs_delegate_tasked(g_thread* delegate_thread) :
		transaction_storage_phys(0), delegate_thread(delegate_thread) {

}

/**
 *
 */
bool g_fs_delegate_tasked::prepare(g_virtual_address* out_transaction_storage) {

	g_virtual_address transaction_storage_address = delegate_thread->process->virtualRanges.allocate(1, G_PROC_VIRTUAL_RANGE_FLAG_PHYSICAL_OWNER);
	if (transaction_storage_address == 0) {
		g_log_warn("%! failed to allocate virtual range for transaction storage when trying to create tasked delegate", "filesystem");
		return false;
	}

	transaction_storage_phys = g_pp_allocator::allocate();
	if (transaction_storage_phys == 0) {
		g_log_warn("%! failed to allocate physical page for transaction storage when trying to create tasked delegate", "filesystem");
		return false;
	}

	/**
	 * For safety, were switching to the page directory of the process
	 * that is registered as a delegate and then map the transaction storage.
	 *
	 * This is to make sure that, if I get the amazing idea to one day trigger a
	 * file system delegate creation from within another process, the world will
	 * not fall into pieces.
	 */
	g_page_directory current = g_address_space::get_current_space();
	g_address_space::switch_to_space(delegate_thread->process->pageDirectory);
	g_address_space::map(transaction_storage_address, transaction_storage_phys, DEFAULT_USER_TABLE_FLAGS, DEFAULT_USER_PAGE_FLAGS);
	g_address_space::switch_to_space(current);
	g_log_debug("%! fs delegate transaction storage created at %h of process %i", "filesystem", transaction_storage_address, delegate_thread->id);

	*out_transaction_storage = transaction_storage_address;
	transaction_storage.set((void*) transaction_storage_address, delegate_thread->process->pageDirectory);
	return true;
}

/**
 *
 */
g_fs_transaction_id g_fs_delegate_tasked::request_discovery(g_thread* requester, g_fs_node* parent, char* child, g_fs_transaction_handler_discovery* handler) {

	// begin the transaction
	g_fs_transaction_id id = g_fs_transaction_store::next_transaction();

	/**
	 * Switch to the delegates space and fill the transaction storage.
	 */
	bool configuration_fine = true;
	g_page_directory current = g_address_space::get_current_space();
	g_address_space::switch_to_space(delegate_thread->process->pageDirectory);

	g_fs_tasked_delegate_transaction_storage_discovery* disc = (g_fs_tasked_delegate_transaction_storage_discovery*) transaction_storage();
	int childlen = g_string::length(child);
	if (childlen > G_FILENAME_MAX) {
		g_log_info("tried to discover a node that has a name with an illegal length");
		configuration_fine = false;
	} else {
		g_memory::copy(disc->name, child, childlen + 1);

		disc->parent_phys_fs_id = parent->phys_fs_id;
	}

	g_address_space::switch_to_space(current);

	// update the status / notify delegate thread
	if (configuration_fine == false) {
		handler->status = G_FS_DISCOVERY_ERROR;

	} else {
		// send message to the task delegate
		/*
		 TODO
		 g_message_empty(request);
		 request.type = G_FS_TASKED_DELEGATE_REQUEST_TYPE_DISCOVER;
		 request.parameterA = id;

		 int send_status = g_message_controller::send(delegate_thread->id, &request);
		 */
		g_message_send_status send_status = G_MESSAGE_SEND_STATUS_FAILED; // TODO

		if (send_status == G_MESSAGE_SEND_STATUS_SUCCESSFUL) {
			// task was requested, wait for answer
			g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_WAITING);

		} else if (send_status == G_MESSAGE_SEND_STATUS_QUEUE_FULL) {
			g_log_warn("%! message queue was full when trying to request discovery to fs delegate", "filesystem");
			handler->status = G_FS_DISCOVERY_BUSY;
			g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);

		} else if (send_status == G_MESSAGE_SEND_STATUS_FAILED) {
			g_log_warn("%! message sending failed when trying to request discovery to fs delegate", "filesystem");
			handler->status = G_FS_DISCOVERY_ERROR;
			g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);
		}
	}

	return id;
}

/**
 *
 */
void g_fs_delegate_tasked::finish_discovery(g_thread* requester, g_fs_transaction_handler_discovery* handler) {

	// save current directory
	g_page_directory current = g_address_space::get_current_space();

	// First get status from transaction storage
	g_address_space::switch_to_space(delegate_thread->process->pageDirectory);
	g_fs_tasked_delegate_transaction_storage_discovery* dspace = (g_fs_tasked_delegate_transaction_storage_discovery*) transaction_storage();
	g_fs_discovery_status status = dspace->result_status;

	// Now switch to the requesters space and copy status there
	g_address_space::switch_to_space(requester->process->pageDirectory);
	handler->status = status;
	g_address_space::switch_to_space(current);
}

/**
 *
 */
g_fs_transaction_id g_fs_delegate_tasked::request_read(g_thread* requester, g_fs_node* node, int64_t length, g_contextual<uint8_t*> buffer,
		g_file_descriptor_content* fd, g_fs_transaction_handler_read* handler) {

	// start/repeat transaction
	g_fs_transaction_id id;
	if (handler->wants_repeat_transaction()) {
		id = handler->get_repeated_transaction();
	} else {
		id = g_fs_transaction_store::next_transaction();
	}

	// save current directory
	g_page_directory current = g_address_space::get_current_space();

	/**
	 * First we switch to the requesters space and copy everything that we need below to the stack,
	 * then get the physical addresses of the area where the requesters buffer is:
	 */
	g_address_space::switch_to_space(requester->process->pageDirectory);

	int required_pages = G_PAGE_ALIGN_UP(length) / G_PAGE_SIZE + 1;
	g_local<g_physical_address> phys_pages(new g_physical_address[required_pages]);
	g_virtual_address virt_start = G_PAGE_ALIGN_DOWN((g_virtual_address ) buffer());
	uint32_t offset_in_first_page = ((g_virtual_address) buffer()) & G_PAGE_ALIGN_MASK;

	for (int i = 0; i < required_pages; i++) {
		phys_pages()[i] = g_address_space::virtual_to_physical(virt_start + i * G_PAGE_SIZE);
	}

	/**
	 * Now we switch into the delegates space, copy the required data to the
	 * transaction storage and map all physical pages from the requesters space
	 * to the delegates space.
	 */
	g_address_space::switch_to_space(delegate_thread->process->pageDirectory);

	g_fs_tasked_delegate_transaction_storage_read* disc = (g_fs_tasked_delegate_transaction_storage_read*) transaction_storage();
	disc->offset = fd->offset;
	disc->length = length;
	disc->phys_fs_id = node->phys_fs_id;

	g_virtual_address mapped_virt = delegate_thread->process->virtualRanges.allocate(required_pages);
	for (int i = 0; i < required_pages; i++) {
		g_address_space::map(mapped_virt + i * G_PAGE_SIZE, phys_pages()[i], DEFAULT_USER_TABLE_FLAGS, DEFAULT_USER_PAGE_FLAGS);
	}
	disc->mapping_start = mapped_virt;
	disc->mapping_pages = required_pages;
	disc->mapped_buffer = (void*) (mapped_virt + offset_in_first_page);

	g_address_space::switch_to_space(current);

	// send message to the task delegate
	/* TODO
	 g_message_empty(request);
	 request.type = G_FS_TASKED_DELEGATE_REQUEST_TYPE_READ;
	 request.parameterA = id;

	 g_message_send_status send_status = g_message_controller::send(delegate_thread->id, &request);
	 */
	g_message_send_status send_status = G_MESSAGE_SEND_STATUS_FAILED; // TODO

	if (send_status == G_MESSAGE_SEND_STATUS_SUCCESSFUL) {
		// set transaction status
		g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_WAITING);

	} else if (send_status == G_MESSAGE_SEND_STATUS_QUEUE_FULL) {
		g_log_warn("%! message queue was full when trying to request read to fs delegate", "filesystem");
		handler->status = G_FS_READ_BUSY;
		g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);

	} else if (send_status == G_MESSAGE_SEND_STATUS_FAILED) {
		g_log_warn("%! message sending failed when trying to request read to fs delegate", "filesystem");
		handler->status = G_FS_READ_ERROR;
		g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);
	}

	return id;

}

/**
 *
 */
void g_fs_delegate_tasked::finish_read(g_thread* requester, g_fs_read_status* out_status, int64_t* out_result, g_file_descriptor_content* fd) {

	// save current directory
	g_page_directory current = g_address_space::get_current_space();

	// Get values from transaction storage and unmap the mapping
	g_address_space::switch_to_space(delegate_thread->process->pageDirectory);

	g_fs_tasked_delegate_transaction_storage_read* rspace = (g_fs_tasked_delegate_transaction_storage_read*) transaction_storage();
	int64_t length_read = rspace->result_read;
	g_fs_read_status status = rspace->result_status;

	for (int i = 0; i < rspace->mapping_pages; i++) {
		g_address_space::unmap(rspace->mapping_start + i * G_PAGE_SIZE);
	}
	delegate_thread->process->virtualRanges.free(rspace->mapping_start);

	// Now switch to the requesters space and copy data there
	g_address_space::switch_to_space(requester->process->pageDirectory);

	*out_result = length_read;
	*out_status = status;
	if (length_read >= 0) {
		fd->offset += length_read;
	}

	g_address_space::switch_to_space(current);
}

/**
 *
 */
g_fs_transaction_id g_fs_delegate_tasked::request_write(g_thread* requester, g_fs_node* node, int64_t length, g_contextual<uint8_t*> buffer,
		g_file_descriptor_content* fd, g_fs_transaction_handler_write* handler) {

	// start/repeat transaction
	g_fs_transaction_id id;
	if (handler->wants_repeat_transaction()) {
		id = handler->get_repeated_transaction();
	} else {
		id = g_fs_transaction_store::next_transaction();
	}

	// save current directory
	g_page_directory current = g_address_space::get_current_space();

	/**
	 * First we switch to the requesters space and copy everything that we need below to the stack,
	 * then get the physical addresses of the area where the requesters buffer is:
	 */
	g_address_space::switch_to_space(requester->process->pageDirectory);

	int required_pages = G_PAGE_ALIGN_UP(length) / G_PAGE_SIZE + 1;
	g_local<g_physical_address> phys_pages(new g_physical_address[required_pages]);
	g_virtual_address virt_start = G_PAGE_ALIGN_DOWN((g_virtual_address ) buffer());
	uint32_t offset_in_first_page = ((g_virtual_address) buffer()) & G_PAGE_ALIGN_MASK;

	for (int i = 0; i < required_pages; i++) {
		phys_pages()[i] = g_address_space::virtual_to_physical(virt_start + i * G_PAGE_SIZE);
	}

	/**
	 * Now we switch into the delegates space, copy the required data to the
	 * transaction storage and map all physical pages from the requesters space
	 * to the delegates space.
	 */
	g_address_space::switch_to_space(delegate_thread->process->pageDirectory);

	g_fs_tasked_delegate_transaction_storage_write* disc = (g_fs_tasked_delegate_transaction_storage_write*) transaction_storage();
	disc->offset = fd->offset;
	disc->length = length;
	disc->phys_fs_id = node->phys_fs_id;

	g_virtual_address mapped_virt = delegate_thread->process->virtualRanges.allocate(required_pages);
	for (int i = 0; i < required_pages; i++) {
		g_address_space::map(mapped_virt + i * G_PAGE_SIZE, phys_pages()[i], DEFAULT_USER_TABLE_FLAGS, DEFAULT_USER_PAGE_FLAGS);
	}
	disc->mapping_start = mapped_virt;
	disc->mapping_pages = required_pages;
	disc->mapped_buffer = (void*) (mapped_virt + offset_in_first_page);

	g_address_space::switch_to_space(current);

	// send message to the task delegate
	/* TODO
	 g_message_empty(request);
	 request.type = G_FS_TASKED_DELEGATE_REQUEST_TYPE_WRITE;
	 request.parameterA = id;

	 g_message_send_status send_status = g_message_controller::send(delegate_thread->id, &request);
	 */
	g_message_send_status send_status = G_MESSAGE_SEND_STATUS_FAILED; // TODO

	if (send_status == G_MESSAGE_SEND_STATUS_SUCCESSFUL) {

		// set transaction status
		g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_WAITING);

	} else if (send_status == G_MESSAGE_SEND_STATUS_QUEUE_FULL) {
		g_log_warn("%! message queue was full when trying to request write to fs delegate", "filesystem");
		handler->status = G_FS_WRITE_BUSY;
		g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);

	} else if (send_status == G_MESSAGE_SEND_STATUS_FAILED) {
		g_log_warn("%! message sending failed when trying to request write to fs delegate", "filesystem");
		handler->status = G_FS_WRITE_ERROR;
		g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);
	}

	return id;

}

/**
 *
 */
void g_fs_delegate_tasked::finish_write(g_thread* requester, g_fs_write_status* out_status, int64_t* out_result, g_file_descriptor_content* fd) {

	// save current directory
	g_page_directory current = g_address_space::get_current_space();

	// Get values from transaction storage and unmap the mapping
	g_address_space::switch_to_space(delegate_thread->process->pageDirectory);

	g_fs_tasked_delegate_transaction_storage_write* storage = (g_fs_tasked_delegate_transaction_storage_write*) transaction_storage();
	int64_t length_write = storage->result_write;
	g_fs_read_status status = storage->result_status;

	for (int i = 0; i < storage->mapping_pages; i++) {
		g_address_space::unmap(storage->mapping_start + i * G_PAGE_SIZE);
	}
	delegate_thread->process->virtualRanges.free(storage->mapping_start);

	// Now switch to the requesters space and copy data there
	g_address_space::switch_to_space(requester->process->pageDirectory);

	*out_result = length_write;
	*out_status = status;
	if (length_write >= 0) {
		fd->offset += length_write;
	}

	g_address_space::switch_to_space(current);
}

/**
 *
 */
g_fs_transaction_id g_fs_delegate_tasked::request_get_length(g_thread* requester, g_fs_node* node, g_fs_transaction_handler_get_length* handler) {

	// the ramdisk handler is doing it's work immediately and doesn't request another process
	g_fs_transaction_id id = g_fs_transaction_store::next_transaction();

	/**
	 * Switch to the delegates space and fill the transaction storage.
	 */
	g_page_directory current = g_address_space::get_current_space();
	g_address_space::switch_to_space(delegate_thread->process->pageDirectory);

	g_fs_tasked_delegate_transaction_storage_get_length* disc = (g_fs_tasked_delegate_transaction_storage_get_length*) transaction_storage();
	disc->phys_fs_id = node->phys_fs_id;

	g_address_space::switch_to_space(current);

	// update the status / notify delegate thread
	/* TODO
	 g_message_empty (request);
	 request.type = G_FS_TASKED_DELEGATE_REQUEST_TYPE_GET_LENGTH;
	 request.parameterA = id;

	 int send_status = g_message_controller::send(delegate_thread->id, &request);
	 */
	g_message_send_status send_status = G_MESSAGE_SEND_STATUS_FAILED; // TODO

	if (send_status == G_MESSAGE_SEND_STATUS_SUCCESSFUL) {
		// task was requested, wait for answer
		g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_WAITING);

	} else if (send_status == G_MESSAGE_SEND_STATUS_QUEUE_FULL) {
		g_log_warn("%! message queue was full when trying to request get-length to fs delegate", "filesystem");
		handler->status = G_FS_LENGTH_BUSY;
		g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);

	} else if (send_status == G_MESSAGE_SEND_STATUS_FAILED) {
		g_log_warn("%! message sending failed when trying to request get-length to fs delegate", "filesystem");
		handler->status = G_FS_LENGTH_ERROR;
		g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);
	}

	return id;
}

/**
 *
 */
void g_fs_delegate_tasked::finish_get_length(g_thread* requester, g_fs_transaction_handler_get_length* handler) {

	// save current directory
	g_page_directory current = g_address_space::get_current_space();

	// Get values from transaction storage and unmap the mapping
	g_address_space::switch_to_space(delegate_thread->process->pageDirectory);

	g_fs_tasked_delegate_transaction_storage_get_length* storage = (g_fs_tasked_delegate_transaction_storage_get_length*) transaction_storage();
	int64_t length = storage->result_length;
	g_fs_read_status status = storage->result_status;

	// Now switch to the requesters space and copy data there
	g_address_space::switch_to_space(requester->process->pageDirectory);

	handler->length = length;
	handler->status = status;

	g_address_space::switch_to_space(current);
}

/**
 *
 */
g_fs_transaction_id g_fs_delegate_tasked::request_directory_refresh(g_thread* requester, g_fs_node* node, g_fs_transaction_handler_directory_refresh* handler) {

	g_fs_transaction_id id = g_fs_transaction_store::next_transaction();

	/**
	 * Switch to the delegates space and fill the transaction storage.
	 */
	g_page_directory current = g_address_space::get_current_space();
	g_address_space::switch_to_space(delegate_thread->process->pageDirectory);

	g_fs_tasked_delegate_transaction_storage_directory_refresh* disc = (g_fs_tasked_delegate_transaction_storage_directory_refresh*) transaction_storage();
	disc->parent_phys_fs_id = node->phys_fs_id;
	disc->parent_virt_fs_id = node->id;

	g_address_space::switch_to_space(current);

	// update the status / notify delegate thread
	/*
	 g_message_empty (request);
	 request.type = G_FS_TASKED_DELEGATE_REQUEST_TYPE_READ_DIRECTORY;
	 request.parameterA = id;

	 int send_status = g_message_controller::send(delegate_thread->id, &request);
	 */
	g_message_send_status send_status = G_MESSAGE_SEND_STATUS_FAILED; // TODO

	if (send_status == G_MESSAGE_SEND_STATUS_SUCCESSFUL) {
		// task was requested, wait for answer
		g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_WAITING);

	} else if (send_status == G_MESSAGE_SEND_STATUS_QUEUE_FULL) {
		g_log_warn("%! message queue was full when trying to request read-directory on fs delegate", "filesystem");
		handler->status = G_FS_DIRECTORY_REFRESH_BUSY;
		g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);

	} else if (send_status == G_MESSAGE_SEND_STATUS_FAILED) {
		g_log_warn("%! message sending failed when trying to request read-directory on fs delegate", "filesystem");
		handler->status = G_FS_DIRECTORY_REFRESH_ERROR;
		g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);
	}

	return id;
}

/**
 *
 */
void g_fs_delegate_tasked::finish_directory_refresh(g_thread* requester, g_fs_transaction_handler_directory_refresh* handler) {

	// save current directory
	g_page_directory current = g_address_space::get_current_space();

	// Get values from transaction storage and unmap the mapping
	g_address_space::switch_to_space(delegate_thread->process->pageDirectory);

	g_fs_tasked_delegate_transaction_storage_directory_refresh* storage = (g_fs_tasked_delegate_transaction_storage_directory_refresh*) transaction_storage();
	g_fs_directory_refresh_status status = storage->result_status;
	g_fs_virt_id parent_id = storage->parent_virt_fs_id;

	// Now switch to the requesters space and copy data there
	g_address_space::switch_to_space(requester->process->pageDirectory);

	// update the folder
	g_fs_node* folder = g_filesystem::get_node_by_id(parent_id);
	if (folder) {
		folder->contents_valid = true;
	}

	handler->status = status;

	g_address_space::switch_to_space(current);
}

/**
 *
 */
g_fs_transaction_id g_fs_delegate_tasked::request_open(g_thread* requester, g_fs_node* node, char* filename, int32_t flags, int32_t mode,
		g_fs_transaction_handler_open* handler) {

	// the ramdisk handler is doing it's work immediately and doesn't request another process
	g_fs_transaction_id id = g_fs_transaction_store::next_transaction();
	bool configuration_fine = true;

	/**
	 * Switch to the delegates space and fill the transaction storage.
	 */
	g_page_directory current = g_address_space::get_current_space();
	g_address_space::switch_to_space(delegate_thread->process->pageDirectory);

	g_fs_tasked_delegate_transaction_storage_open* disc = (g_fs_tasked_delegate_transaction_storage_open*) transaction_storage();
	disc->phys_fs_id = node->phys_fs_id;

	int childlen = g_string::length(filename);
	if (childlen > G_FILENAME_MAX) {
		g_log_info("tried to open a node that has a name with an illegal length");
		configuration_fine = false;
	} else {
		g_memory::copy(disc->name, filename, childlen + 1);
	}

	g_address_space::switch_to_space(current);

	if (!configuration_fine) {
		handler->status = G_FS_OPEN_ERROR;
		g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);
		return id;
	}

	// update the status / notify delegate thread
	/* TODO
	 g_message_empty (request);
	 request.type = G_FS_TASKED_DELEGATE_REQUEST_TYPE_OPEN;
	 request.parameterA = id;

	 int send_status = g_message_controller::send(delegate_thread->id, &request);
	 */
	g_message_send_status send_status = G_MESSAGE_SEND_STATUS_FAILED; // TODO

	if (send_status == G_MESSAGE_SEND_STATUS_SUCCESSFUL) {
		// task was requested, wait for answer
		g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_WAITING);

	} else if (send_status == G_MESSAGE_SEND_STATUS_QUEUE_FULL) {
		g_log_warn("%! message queue was full when trying to request open to fs delegate", "filesystem");
		handler->status = G_FS_OPEN_BUSY;
		g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);

	} else if (send_status == G_MESSAGE_SEND_STATUS_FAILED) {
		g_log_warn("%! message sending failed when trying to request open to fs delegate", "filesystem");
		handler->status = G_FS_OPEN_ERROR;
		g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);
	}

	return id;
}

/**
 *
 */
void g_fs_delegate_tasked::finish_open(g_thread* requester, g_fs_transaction_handler_open* handler) {

	// save current directory
	g_page_directory current = g_address_space::get_current_space();

	// Get values from transaction storage and unmap the mapping
	g_address_space::switch_to_space(delegate_thread->process->pageDirectory);

	g_fs_tasked_delegate_transaction_storage_open* storage = (g_fs_tasked_delegate_transaction_storage_open*) transaction_storage();
	g_fs_open_status status = storage->result_status;

	// Now switch to the requesters space and copy data there
	g_address_space::switch_to_space(requester->process->pageDirectory);

	handler->status = status;

	g_address_space::switch_to_space(current);
}

/**
 *
 */
g_fs_transaction_id g_fs_delegate_tasked::request_close(g_thread* requester, g_fs_transaction_handler_close* handler, g_file_descriptor_content* fd,
		g_fs_node* node) {

	g_fs_transaction_id id = g_fs_transaction_store::next_transaction();

	// fill transaction storage
	g_page_directory current = g_address_space::get_current_space();
	g_address_space::switch_to_space(delegate_thread->process->pageDirectory);

	g_fs_tasked_delegate_transaction_storage_close* disc = (g_fs_tasked_delegate_transaction_storage_close*) transaction_storage();
	disc->phys_fs_id = node->phys_fs_id;

	g_address_space::switch_to_space(current);

	// update the status / notify delegate thread
	/* TODO
	 g_message_empty (request);
	 request.type = G_FS_TASKED_DELEGATE_REQUEST_TYPE_CLOSE;
	 request.parameterA = id;

	 int send_status = g_message_controller::send(delegate_thread->id, &request);
	 */
	g_message_send_status send_status = G_MESSAGE_SEND_STATUS_FAILED; // TODO

	if (send_status == G_MESSAGE_SEND_STATUS_SUCCESSFUL) {
		// task was requested, wait for answer
		g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_WAITING);

	} else if (send_status == G_MESSAGE_SEND_STATUS_QUEUE_FULL) {
		g_log_warn("%! message queue was full when trying to request close on fs delegate", "filesystem");
		handler->status = G_FS_CLOSE_BUSY;
		g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);

	} else if (send_status == G_MESSAGE_SEND_STATUS_FAILED) {
		g_log_warn("%! message sending failed when trying to request close on fs delegate", "filesystem");
		handler->status = G_FS_CLOSE_ERROR;
		g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);
	}

	return id;
}

/**
 *
 */
void g_fs_delegate_tasked::finish_close(g_thread* requester, g_fs_transaction_handler_close* handler) {

	g_page_directory current = g_address_space::get_current_space();

	// copy values from transaction storage to local stack
	g_address_space::switch_to_space(delegate_thread->process->pageDirectory);

	g_fs_tasked_delegate_transaction_storage_close* storage = (g_fs_tasked_delegate_transaction_storage_close*) transaction_storage();
	g_fs_directory_refresh_status status = storage->result_status;

	// copy values to the handler
	g_address_space::switch_to_space(requester->process->pageDirectory);
	handler->status = status;

	// switch back
	g_address_space::switch_to_space(current);
}

