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

g_fs_delegate_status g_fs_delegate_unregister(g_fs_virt_id mountpoint_id)
{
	if(mountpoint_id < 0)
		return G_FS_DELEGATE_INVALID_ARGUMENT;

	g_syscall_fs_delegate_unregister data{};
	data.mountpoint_id = mountpoint_id;
	data.result = G_FS_DELEGATE_UNSUPPORTED;
	g_syscall(G_SYSCALL_FS_DELEGATE_UNREGISTER, (g_address) &data);

	return data.result;
}
