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

#include <multiboot/multiboot_module_analyzer.hpp>

#include <loader.hpp>
#include <logger/logger.hpp>

/**
 * 
 */
uint32_t g_multiboot_module_analyzer::calculateEndOfHighestModule() {

	g_log_debug("%! searching for modules", "multiboot");
	uint32_t highestEnd = 0;

	// Check if modules are available
	g_multiboot_information* mbInfo = g_loader::getSetupInformation()->multibootInformation;
	if (mbInfo->flags & (1 << 3)) {

		uint32_t moduleCount = mbInfo->modulesCount;
		for (uint32_t i = 0; i < moduleCount; i++) {

			g_multiboot_module* module = (g_multiboot_module*) (mbInfo->modulesAddress + sizeof(g_multiboot_module) * i);
			g_log_debug("%#   %h - %h: %s ", module->moduleStart, module->moduleEnd, module->path);

			// Check modules end address
			if (module->moduleEnd > highestEnd) {
				highestEnd = module->moduleEnd;
			}
		}

	} else {
		g_loader::panic("%! modules are not available", "multiboot");
	}

	g_log_debug("%! highest module end address is %h", "multiboot", highestEnd);
	return highestEnd;
}
