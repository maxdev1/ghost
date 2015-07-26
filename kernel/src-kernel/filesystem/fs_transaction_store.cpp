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

#include "filesystem/fs_transaction_store.hpp"

#include "utils/hash_map.hpp"

static g_fs_transaction_id next_transaction_id = 0;
static g_hash_map<g_fs_transaction_id, g_fs_transaction_status>* store;

/**
 *
 */
void g_fs_transaction_store::initialize() {
	store = new g_hash_map<g_fs_transaction_id, g_fs_transaction_status>();
}

/**
 *
 */
g_fs_transaction_id g_fs_transaction_store::next_transaction() {
	return next_transaction_id++;
}

/**
 *
 */
g_fs_transaction_status g_fs_transaction_store::get_status(g_fs_transaction_id id) {

	auto entry = store->get(id);
	if (entry) {
		return entry->value;
	}
	return 0;
}

/**
 *
 */
void g_fs_transaction_store::set_status(g_fs_transaction_id id, g_fs_transaction_status result) {

	store->put(id, result);
}

/**
 *
 */
void g_fs_transaction_store::remove_transaction(g_fs_transaction_id id) {

	store->remove(id);
}
