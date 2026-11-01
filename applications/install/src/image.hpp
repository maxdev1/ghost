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

#ifndef GHOST_INSTALL_IMAGE
#define GHOST_INSTALL_IMAGE

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

constexpr size_t G_INSTALL_SECTOR_SIZE = 512;
constexpr size_t G_INSTALL_BUFFER_SIZE = 4096;
constexpr size_t G_INSTALL_BOOT_SIZE = 65536;
constexpr uint32_t G_INSTALL_FIRST_SECTOR = 2048;
constexpr uint64_t G_INSTALL_MIN_SECTORS = 131072;
constexpr const char* G_INSTALL_BOOT_PATH = "/system/install/limine.bin";

struct g_install_entry
{
	std::string path;
	uint64_t size = 0;
	size_t parent = 0;
	bool directory = false;
	std::string sourcePath{};
};

struct g_install_plan
{
	uint32_t serial = 0;
	uint32_t firstSector = G_INSTALL_FIRST_SECTOR;
	uint64_t bytes = 0;
	std::vector<g_install_entry> entries;
};

using g_install_progress = std::function<void(const char*, uint64_t, uint64_t)>;

bool installBootValidate(const uint8_t* boot, size_t length, const char** error);

#endif
