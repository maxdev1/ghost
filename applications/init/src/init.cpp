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

#include <ghost.h>
#include <ghostuser/utils/logger.hpp>

/**
 *
 */
int main(int argc, char** argv) {

	int x = 0;
	int bla = 0;
	for (;;) {
		klog("Hello world!");
		g_sleep(5000);
	}

	/*
	 g_task_register_id("init");

	 // load spawner process
	 g_ramdisk_spawn_status spawner_stat = g_ramdisk_spawn("applications/spawner.bin", G_SECURITY_LEVEL_KERNEL);
	 if (spawner_stat != G_RAMDISK_SPAWN_STATUS_SUCCESSFUL) {
	 g_logger::log("unable to load system spawner process");
	 yield: asm("hlt");
	 goto yield;
	 }

	 // wait for spawner to get ready
	 g_tid spawner_id;
	 while ((spawner_id = g_task_get_id(G_SPAWNER_IDENTIFIER)) == -1) {
	 g_yield();
	 }

	 // let the spawner load the launch service
	 g_pid ls_pid;
	 std::string launch_srv_path = "/applications/launch.bin";

	 g_spawn_status stat = g_spawn_p(launch_srv_path.c_str(), "/system/launch/init", "/", G_SECURITY_LEVEL_KERNEL, &ls_pid);
	 if (stat == G_SPAWN_STATUS_SUCCESSFUL) {
	 g_logger::log("launch service executed in process %i", ls_pid);
	 } else {
	 g_logger::log("failed to load launch service from '" + launch_srv_path + "' with code %i", stat);
	 }
	 */
}
