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

#include "shared/multiboot/multiboot.hpp"
#include "shared/logger/logger.hpp"
#include "shared/utils/string.hpp"

g_multiboot_module* multibootFindModule(g_multiboot_information* info, const char* path)
{
	logDebug("%! searching for module: %s", "mbutil", path);

	uint32_t moduleCount = info->modulesCount;

	for(uint32_t i = 0; i < moduleCount; i++)
	{
		g_multiboot_module* module = (g_multiboot_module*) (info->modulesAddress + sizeof(g_multiboot_module) * i);
		logDebug("%#  module: %s", module->path);

		// Check modules path
		if(stringEquals(module->path, path))
			return module;
	}

	return 0;
}

