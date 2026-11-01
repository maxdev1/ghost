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

#include <cstring>
#include <ghost/tasks.h>
#include <libstorage/admin.hpp>
#include "administration/administration.hpp"
#include "volume/mount.hpp"

int main(int argc, char** argv)
{
	if(argc == 3 && !strcmp(argv[1], "--root-disk") && !fatMountConfigureRoot(argv[2]))
		return 1;

	if(!g_task_register_name(G_FAT_DRIVER_NAME) ||
	   g_create_task(reinterpret_cast<void*>(fatDiscoverVolumes)) == G_TID_NONE)
		return 1;

	g_task_await_by_name(G_STORAGE_MANAGER_NAME);
	if(storageManagerFormatterRegister("fat32") != G_VOLUME_OK)
		return 1;

	uint8_t incoming[sizeof(g_message_header) + G_MESSAGE_MAXIMUM_MESSAGE_LENGTH];
	while(g_receive_message(incoming, sizeof(incoming)) == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
	{
		auto message = (g_message_header*) incoming;
		fatAdminReceive(message, G_MESSAGE_CONTENT(incoming));
	}
	return 1;
}
