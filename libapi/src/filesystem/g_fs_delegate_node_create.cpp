/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2026, Max Schlüssel <lokoxe@gmail.com>                     *
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

#include "ghost/filesystem.h"
#include "ghost/filesystem/callstructs.h"
#include "ghost/syscall.h"

g_fs_delegate_status g_fs_delegate_node_create(g_fs_transaction_id transaction, g_fs_virt_id parent, const char* name,
											   g_fs_node_type type, g_fs_phys_id phys_fs_id,
											   g_fs_virt_id* out_created_id)
{
	if(out_created_id)
		*out_created_id = G_FS_VIRT_ID_NONE;
	if(transaction == G_FS_TRANSACTION_NONE || parent < 0 || !name || !*name || !out_created_id ||
	   (type != G_FS_NODE_TYPE_FILE && type != G_FS_NODE_TYPE_FOLDER))
		return G_FS_DELEGATE_INVALID_ARGUMENT;

	g_syscall_fs_delegate_node_create data{};
	data.transaction = transaction;
	data.parent_id = parent;
	data.name = name;
	data.type = type;
	data.phys_fs_id = phys_fs_id;
	data.created_id = G_FS_VIRT_ID_NONE;
	data.result = G_FS_DELEGATE_UNSUPPORTED;
	g_syscall(G_SYSCALL_FS_DELEGATE_NODE_CREATE, (g_address) &data);

	if(data.result == G_FS_DELEGATE_SUCCESSFUL)
		*out_created_id = data.created_id;
	return data.result;
}
