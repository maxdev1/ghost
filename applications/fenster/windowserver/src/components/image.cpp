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

#include "components/image.hpp"
#include <libwindow/font/font_loader.hpp>
#include <libwindow/properties.hpp>

#include <cairo/cairo.h>
#include <sstream>
#include "windowserver.hpp"

image_t::image_t()
{
}

void image_t::loadImage(std::string path)
{
	platformAcquireMutex(this->lock);
	if(image)
	{
		cairo_surface_destroy(image);
		image = nullptr;
	}

	image = cairo_image_surface_create_from_png(path.c_str());
	if(cairo_surface_status(image) != CAIRO_STATUS_SUCCESS)
	{
		platformLog("failed to load image: %s", path.c_str());
		image = nullptr;
	}
	platformReleaseMutex(this->lock);

	markFor(COMPONENT_REQUIREMENT_PAINT);
}

bool image_t::setStringProperty(int property, std::string text)
{
	if(property == G_UI_PROPERTY_IMAGE_SOURCE)
	{
		this->loadImage(text);
		return true;
	}
	return false;
}

void image_t::paint()
{
	auto cr = graphics.acquireContext();
	if(!cr)
		return;

	platformAcquireMutex(this->lock);

	cairo_save(cr);
	int imageWidth = cairo_image_surface_get_width(image);
	int imageX = bounds.width / 2 - imageWidth / 2;
	int imageY = 10;
	cairo_set_source_surface(cr, image, imageX, imageY);
	cairo_rectangle(cr, imageX, imageY, bounds.width, bounds.height);
	cairo_fill(cr);
	cairo_restore(cr);

	platformReleaseMutex(this->lock);

	graphics.releaseContext();

}
