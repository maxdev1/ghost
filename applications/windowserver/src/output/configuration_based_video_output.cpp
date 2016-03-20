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

#include "configuration_based_video_output.hpp"
#include <ghostuser/utils/property_file_parser.hpp>
#include <fstream>
#include <iostream>
#include <sstream>

/**
 *
 */
bool configuration_based_video_output_t::initialize() {

	// open configuration file
	std::string path = "/system/graphics/configuration/resolution.cfg";
	std::ifstream in(path);
	if (!in.good()) {
		std::cerr << "unable to read configuration file: '" << path << "'" << std::endl;
		return false;
	}

	// parse it
	g_property_file_parser parser(in);
	auto properties = parser.getProperties();

	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t bpp = 0;

	for (auto entry : properties) {
		std::stringstream s;
		s << entry.second;

		if (entry.first == "width") {
			s >> width;
		} else if (entry.first == "height") {
			s >> height;
		} else if (entry.first == "bpp") {
			s >> bpp;
		}
	}

	// check settings
	if (width == 0 || height == 0 || bpp == 0) {
		std::cerr << "invalid configuration. width: " << width << ", height: " << height << ", bpp: " << bpp << std::endl;
		return false;
	}

	// call initialization
	return initialize_with_settings(width, height, bpp);
}
