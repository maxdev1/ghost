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

#include "ghost/user.h"

/**
 *
 */
int g_fs_register_as_delegate(const char* name, g_fs_phys_id phys_mountpoint_id, g_fs_virt_id* out_mountpoint_id, g_address* out_transaction_storage) {

	g_syscall_fs_register_as_delegate data;
	data.name = (char*) name;
	data.phys_mountpoint_id = phys_mountpoint_id;
	g_syscall(G_SYSCALL_FS_REGISTER_AS_DELEGATE, (g_address) &data);
	*out_mountpoint_id = data.mountpoint_id;
	*out_transaction_storage = data.transaction_storage;
	return data.result;
}
