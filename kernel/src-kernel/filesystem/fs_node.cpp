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

#include "filesystem/fs_node.hpp"
#include "utils/string.hpp"

/**
 *
 */
g_fs_node::g_fs_node() :
		delegate(0), type(G_FS_NODE_TYPE_NONE), id(0), phys_fs_id(0), name(0), parent(0), children(0), is_blocking(true), contents_valid(false) {
}

/**
 *
 */
g_fs_node* g_fs_node::find_child(char* name) {

	g_list_entry<g_fs_node*>* n = children;
	while (n) {
		if (n->value->name != 0 && g_string::equals(n->value->name, name)) {
			return n->value;
		}
		n = n->next;
	}

	return 0;
}

/**
 *
 */
void g_fs_node::set_delegate(g_fs_delegate* delegate) {
	this->delegate = delegate;
}

/**
 *
 */
g_fs_delegate* g_fs_node::get_delegate() {

	if (delegate) {
		return delegate;
	}
	if (parent) {
		return parent->get_delegate();
	}
	return 0;
}

/**
 *
 */
void g_fs_node::add_child(g_fs_node* child) {

	child->parent = this;

	g_list_entry<g_fs_node*>* entry = new g_list_entry<g_fs_node*>();
	entry->value = child;

	entry->next = children;
	children = entry;
}
