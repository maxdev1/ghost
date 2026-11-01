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

g_fs_delegate_status g_fs_delegate_register(const char* name, g_fs_phys_id phys_mountpoint_id,
											g_fs_virt_id* out_mountpoint_id,
											g_fs_delegate_transaction_storage** out_transaction_storage)
{
	if(out_mountpoint_id)
		*out_mountpoint_id = G_FS_VIRT_ID_NONE;

	if(out_transaction_storage)
		*out_transaction_storage = nullptr;

	if(!name || !*name || !out_mountpoint_id || !out_transaction_storage)
		return G_FS_DELEGATE_INVALID_ARGUMENT;

	g_syscall_fs_delegate_register data{};
	data.name = name;
	data.phys_mountpoint_id = phys_mountpoint_id;
	data.version = G_FS_DELEGATE_VERSION;
	data.storage_size = sizeof(g_fs_delegate_transaction_storage);
	data.mountpoint_id = G_FS_VIRT_ID_NONE;
	data.result = G_FS_DELEGATE_UNSUPPORTED;
	g_syscall(G_SYSCALL_FS_DELEGATE_REGISTER, (g_address) &data);

	if(data.result == G_FS_DELEGATE_SUCCESSFUL)
	{
		*out_mountpoint_id = data.mountpoint_id;
		*out_transaction_storage = data.transaction_storage;
	}
	return data.result;
}
