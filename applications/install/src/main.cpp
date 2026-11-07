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

#include <cstdio>
#include <cstring>
#include <ghost.h>
#include "install.hpp"
#include "ui.hpp"

static constexpr const char* G_INSTALL_USAGE =
		"Usage: install list | install [--yes] disk ID | install [--yes] volume ID\n";

enum class g_install_cli_target
{
	List,
	Disk,
	Volume
};

struct g_install_options
{
	g_install_cli_target target = g_install_cli_target::List;
	uint64_t id = 0;
	bool confirmed = false;
};

static bool installCliParseId(const char* text, uint64_t* id)
{
	*id = 0;
	for(auto digit = text; *digit; ++digit)
	{
		if(*digit < '0' || *digit > '9' || *id > (UINT64_MAX - unsigned(*digit - '0')) / 10)
			return false;

		*id = *id * 10 + unsigned(*digit - '0');
	}

	return *id != 0;
}

static bool installCliParseOptions(int argc, char** argv, g_install_options* options)
{
	if(argc == 1 || (argc == 2 && (!strcmp(argv[1], "list") || !strcmp(argv[1], "--list"))))
		return true;

	int first = 1;
	options->confirmed = !strcmp(argv[first], "--yes");
	if(options->confirmed)
		++first;
	options->target = g_install_cli_target::Disk;
	const char* number;
	if(argc == first + 2 && (!strcmp(argv[first], "disk") || !strcmp(argv[first], "volume")))
	{
		if(!strcmp(argv[first], "volume"))
			options->target = g_install_cli_target::Volume;
		number = argv[first + 1];
	}
	else if(argc == first + 1)
		number = argv[first];
	else
		return false;

	if(!installCliParseId(number, &options->id))
		return false;

	return options->target == g_install_cli_target::Volume || options->id <= UINT16_MAX;
}

static void installCliPrintDestination(const g_install_destination& target)
{
	if(target.volume.id)
		printf("  Volume %lu: partition %u, %lu MiB, %s, %s\n", target.volume.id, target.volume.partition,
			   target.volume.sectorCount / 2048, target.volume.filesystem,
			   target.volume.path[0] ? target.volume.path : "unmounted");
	else
		printf("Disk %u: %s, %lu MiB (reformat entire disk)\n", target.disk.id, target.disk.info.model,
			   target.disk.info.sectorCount / 2048);
}

int main(int argc, char** argv)
{
	if(argc == 1 && g_get_launch_mode() == G_LAUNCH_MODE_UI)
		return installUiRun();

	if(argc == 2 && (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")))
	{
		printf("%sDisk targets are reformatted. Volume targets retain their filesystem and other files.\n",
			   G_INSTALL_USAGE);
		return 0;
	}

	g_install_options options;
	if(!installCliParseOptions(argc, argv, &options))
	{
		fprintf(stderr, "%s", G_INSTALL_USAGE);
		return 1;
	}

	for(const auto& target: installDiscoverDestinations())
	{
		if(options.target == g_install_cli_target::List)
			installCliPrintDestination(target);
		else if(options.target == g_install_cli_target::Volume && target.volume.id == options.id)
			return installRun(target, options.confirmed);
		else if(options.target == g_install_cli_target::Disk && !target.volume.id && target.disk.id == options.id)
			return installRun(target, options.confirmed);
	}

	if(options.target != g_install_cli_target::List)
	{
		fprintf(stderr, "install: target not found; run install list\n");
		return 1;
	}

	return 0;
}
