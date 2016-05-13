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

#include "filesystem/pipes.hpp"
#include "logger/logger.hpp"
#include "utils/hash_map.hpp"

/**
 *
 */
static g_pipe_id pipe_next_id = 0;
static g_hash_map<g_pipe_id, g_pipe*>* pipes;

/**
 *
 */
void g_pipes::initialize() {
	pipes = new g_hash_map<g_pipe_id, g_pipe*>();
}

/**
 *
 */
g_pipe_id g_pipes::create() {

	g_pipe_id id = -1;

	g_pipe* pipe = new g_pipe();
	pipe->buffer = new uint8_t[G_PIPE_DEFAULT_CAPACITY];
	pipe->write = pipe->buffer;
	pipe->read = pipe->buffer;
	pipe->size = 0;
	pipe->capacity = G_PIPE_DEFAULT_CAPACITY;
	pipe->references = 0;

	if (pipe->buffer != 0) {
		id = pipe_next_id++;
		pipes->put(id, pipe);
	}

	return id;
}

/**
 *
 */
g_pipe* g_pipes::get(g_pipe_id id) {

	auto entry = pipes->get(id);
	if (entry) {
		return entry->value;
	}
	return 0;
}

/**
 *
 */
void g_pipes::add_reference(g_pipe_id id, g_pid pid) {

	auto pipe_entry = pipes->get(id);
	if (pipe_entry) {
		g_pipe* pipe = pipe_entry->value;

		g_list_entry<g_pid>* entry = new g_list_entry<g_pid>;
		entry->value = pid;
		entry->next = pipe->references;
		pipe->references = entry;
	}
}

/**
 *
 */
bool g_pipes::has_reference_from_other_process(g_pipe* pipe, g_pid pid) {

	g_list_entry<g_pid>* entry = pipe->references;
	while (entry) {
		if (entry->value != pid) {
			return true;
		}
		entry = entry->next;
	}
	return false;
}

/**
 *
 */
void g_pipes::remove_reference(g_pipe_id id, g_pid pid) {

	auto pipe_entry = pipes->get(id);
	if (pipe_entry) {
		g_pipe* pipe = pipe_entry->value;

		// find entry and remove
		g_list_entry<g_pid>* prev = 0;
		g_list_entry<g_pid>* entry = pipe->references;
		while (entry) {
			if (entry->value == pid) {
				if (prev == 0) {
					pipe->references = entry->next;
				} else {
					prev->next = entry->next;
				}
				delete entry;
				break;
			}
			prev = entry;
			entry = entry->next;
		}

		// no entry left?
		if (pipe->references == 0) {
			pipes->remove(id);

			g_log_debug("%! removing non-referenced pipe %i", "pipes", id);
			delete pipe->buffer;
			delete pipe;
		}
	}
}
