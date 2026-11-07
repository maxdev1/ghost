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

#include "volume.hpp"

bool volumeToolParseId(const char* text, g_volume_id& id)
{
	uint64_t value;
	if(!coreParseUnsigned(text, &value) || !value)
		return false;

	id = value;
	return true;
}

bool volumeToolGet(g_volume_id id, g_volume_info& info)
{
	auto status = volumeGet(id, &info);
	if(status == G_VOLUME_OK)
		return true;

	fprintf(stderr, "volume: cannot get volume %llu: %s\n", static_cast<unsigned long long>(id),
			volumeToolStatusText(status));
	return false;
}

/**
 * Resolves numeric IDs directly and other targets against the manager's current
 * mount paths. The returned claim is used to reject later ownership changes.
 */
bool volumeToolResolve(const char* target, g_volume_info& info)
{
	if(coreIsDecimalNumber(target))
	{
		g_volume_id id;
		if(volumeToolParseId(target, id))
			return volumeToolGet(id, info);

		fprintf(stderr, "volume: invalid volume ID '%s'\n", target);
		return false;
	}

	std::string path;
	if(!strcmp(target, "/"))
		path = target;
	else if(!strncmp(target, "/mount/", 7) && volumeMountNameValid(target + 7))
		path = target;
	else if(volumeMountNameValid(target))
		path = std::string("/mount/") + target;
	else
	{
		fprintf(stderr, "volume: invalid mount name or path '%s'\n", target);
		return false;
	}

	g_volume_id after = 0;
	for(;;)
	{
		auto status = volumeNext(after, 0, &info);
		if(status == G_VOLUME_NOT_FOUND)
		{
			fprintf(stderr, "volume: no volume is mounted at '%s'\n", path.c_str());
			return false;
		}

		if(status != G_VOLUME_OK)
		{
			fprintf(stderr, "volume: cannot resolve '%s': %s\n", target, volumeToolStatusText(status));
			return false;
		}

		if(info.state != G_VOLUME_AVAILABLE && path == info.path)
			return true;

		after = info.id;
	}
}
