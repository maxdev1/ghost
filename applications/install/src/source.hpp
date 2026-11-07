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

#ifndef GHOST_INSTALL_SOURCE
#define GHOST_INSTALL_SOURCE

#include "image.hpp"

struct g_install_source_tree
{
	g_install_source_tree() = default;
	g_install_source_tree(const g_install_source_tree&) = delete;
	g_install_source_tree& operator=(const g_install_source_tree&) = delete;
	~g_install_source_tree();

	g_install_plan plan;
	std::string root = "/mount/ramdisk";
	std::string copyError;
	int file = -1;
	size_t current = SIZE_MAX;
	uint64_t patchOffset = UINT64_MAX;
	uint64_t partitionPatchOffset = UINT64_MAX;
	uint64_t (*clock)() = nullptr;
};

bool installSourceCollect(std::vector<g_install_entry>* entries, const char** error,
						  const std::string& root = "/mount/ramdisk");
bool installSourcePrepare(g_install_source_tree* tree, const char** error);
bool installSourceRead(void* context, size_t index, uint64_t offset, uint8_t* bytes, size_t count);

#endif
