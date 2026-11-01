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

#ifndef GHOST_INSTALL
#define GHOST_INSTALL

#include <libstorage/admin.hpp>
#include "image.hpp"

struct g_install_disk
{
	g_device_id id;
	g_disk_info info;
};

struct g_install_destination
{
	g_install_disk disk;
	g_volume_info volume;
};

std::vector<g_install_disk> installDiscoverDisks();

std::vector<g_install_destination> installDiscoverDestinations();

int installRun(const g_install_destination& destination, bool confirmed, std::string* failure = nullptr,
			   g_install_progress progress = nullptr);

#endif
