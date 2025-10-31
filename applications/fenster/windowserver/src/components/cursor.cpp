/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2022, Max Schlüssel <lokoxe@gmail.com>                     *
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

#include "components/cursor.hpp"
#include "windowserver.hpp"

#include <libproperties/parser.hpp>
#include <sstream>

static std::map<std::string, cursor_configuration> cursorConfigurations;
static cursor_configuration* currentConfiguration = 0;

g_point cursor_t::position;
g_point cursor_t::nextPosition;

g_mouse_button cursor_t::pressedButtons = G_MOUSE_EVENT_NONE;
g_mouse_button cursor_t::nextPressedButtons = G_MOUSE_EVENT_NONE;

int cursor_t::nextScroll = 0;

g_ui_component_id cursor_t::pressedComponent = -1;
g_ui_component_id cursor_t::draggedComponent = -1;
g_ui_component_id cursor_t::hoveredComponent = -1;
g_ui_component_id cursor_t::focusedComponent = -1;

void cursor_t::set(std::string name)
{
	if(cursorConfigurations.count(name) > 0)
	{
		currentConfiguration = &cursorConfigurations[name];
	}
	else if(cursorConfigurations.count("default") > 0)
	{
		currentConfiguration = &cursorConfigurations["default"];
	}
	else
	{
		platformLog("could neither load '%s' cursor nor 'default' cursor", name.c_str());
	}

	screen_t* screen = windowserver_t::instance()->screen;
	if(screen)
	{
		screen->markDirty(getArea());
	}
}

std::string cursor_t::get()
{
	if(currentConfiguration)
		return currentConfiguration->name;
	return "default";
}

bool cursor_t::load(std::string cursorPath)
{
	// Open config file
	std::string configpath = cursorPath + "/cursor.cfg";
	std::ifstream in(configpath);
	if(!in.good())
	{
		platformLog("failed to open cursor configuration at %s", configpath.c_str());
		return false;
	}

	g_properties_parser props(in);
	auto content = props.getProperties();

	// Read required params
	std::string name = content["name"];
	std::string hitpoint_x = content["hitpoint.x"];
	std::string hitpoint_y = content["hitpoint.y"];
	std::string image = content["image"];

	if(name.empty() || hitpoint_x.empty() || hitpoint_y.empty() || image.empty())
	{
		platformLog("either name (%s), hitpoint (%s %s), or image (%s) was missing for cursor\n", name.c_str(), hitpoint_x.c_str(), hitpoint_y.c_str(), image.c_str());
		return false;
	}

	// Convert hitpoint
	std::stringstream stx;
	stx << hitpoint_x;
	int hitpointX;
	stx >> hitpointX;

	std::stringstream sty;
	sty << hitpoint_y;
	int hitpointY;
	sty >> hitpointY;

	std::string cursorImagePath = (cursorPath + "/" + image);

	// check if file exists
	FILE* cursorImageFile = fopen(cursorImagePath.c_str(), "r");
	if(cursorImageFile == NULL)
	{
		platformLog("failed to open image file at %s\n", cursorImagePath.c_str());
		return false;
	}
	fclose(cursorImageFile);

	// load cursor
	cursor_configuration pack;
	pack.surface = cairo_image_surface_create_from_png(cursorImagePath.c_str());
	if(pack.surface == nullptr)
	{
		platformLog("failed to load cursor image at '%s' for configuration '%s'", cursorImagePath.c_str(), cursorPath.c_str());
		return false;
	}

	pack.hitpoint = g_point(hitpointX, hitpointY);
	pack.size = g_dimension(cairo_image_surface_get_width(pack.surface), cairo_image_surface_get_height(pack.surface));
	pack.name = name;
	cursorConfigurations[name] = pack;

	return true;
}

void cursor_t::paint(graphics_t* global)
{
	auto cr = global->acquireContext();
	if(!cr)
		return;

	cairo_reset_clip(cr);

	if(currentConfiguration)
	{
		cairo_set_source_surface(cr, currentConfiguration->surface, position.x - currentConfiguration->hitpoint.x,
		                         position.y - currentConfiguration->hitpoint.y);
		cairo_paint(cr);
	}
	else if(cursorConfigurations.size() > 0)
	{
		cairo_set_source_rgb(cr, 0, 0, 0);
		cairo_rectangle(cr, position.x, position.y, FALLBACK_CURSOR_SIZE, FALLBACK_CURSOR_SIZE);
		cairo_fill(cr);
	}

	global->releaseContext();
}

g_rectangle cursor_t::getArea()
{
	if(currentConfiguration)
	{
		return {position.x - currentConfiguration->hitpoint.x,
		        position.y - currentConfiguration->hitpoint.y,
		        currentConfiguration->size.width,
		        currentConfiguration->size.height};
	}

	return {position.x, position.y, FALLBACK_CURSOR_SIZE, FALLBACK_CURSOR_SIZE};
}
