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

#ifndef __CONFIGURATION_BASED_VIDEO_OUTPUT__
#define __CONFIGURATION_BASED_VIDEO_OUTPUT__

#include "video_output.hpp"

/**
 * A configuration based video output reads a configuration file first to
 * determine what video settings shall be used.
 */
class configuration_based_video_output_t: public video_output_t {
public:
	virtual ~configuration_based_video_output_t() {
	}

	/**
	 * Reads the configuration file and then calls the initialization function with
	 * the correct parameters.
	 *
	 * @return whether initialization was successful
	 */
	virtual bool initialize();

	/**
	 * Initializes the video mode with the given settings.
	 *
	 * @param width
	 * 		screen width
	 * @param height
	 * 		screen height
	 * @param bits
	 * 		bit depth
	 *
	 * @return whether initialization was successful
	 */
	virtual bool initialize_with_settings(uint32_t width, uint32_t height, uint32_t bits) = 0;

	/**
	 *
	 */
	virtual void blit(g_rectangle invalid, g_rectangle sourceSize, g_color_argb* source) = 0;

};

#endif
