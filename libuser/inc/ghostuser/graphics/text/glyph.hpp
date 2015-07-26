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

#ifndef GHOSTLIBRARY_GRAPHICS_TEXT_GLYPH
#define GHOSTLIBRARY_GRAPHICS_TEXT_GLYPH

#include <ghostuser/graphics/color_argb.hpp>
#include <ghostuser/graphics/metrics/dimension.hpp>
#include <ghostuser/graphics/metrics/insets.hpp>
#include <ghostuser/graphics/metrics/rectangle.hpp>
#include <ghostuser/graphics/text/freetype.hpp>

/**
 *
 */
class g_glyph {
private:
	g_rectangle bounding;
	FT_Glyph glyph;
	FT_UInt glyphIndex;

	g_color_argb* content;
	g_point bitmapTopLeftPixel;
	g_dimension contentSize;
	g_point advance;

	/**
	 *
	 */
	void copyBitmap(FT_Bitmap* bitmap);

public:

	/**
	 * Creates the glyph object, storing the {FT_Glyph} handle and
	 * copying the contents of the "bitmap" and the "boundingBox".
	 *
	 * @param glyphIndex	freetype glyph index
	 * @param glyph			freetype glyph handle
	 * @param slot			rendered slot to take infos from
	 * @param boundingBox	freetype glyph bounding box
	 */
	g_glyph(FT_UInt glyphIndex, FT_Glyph glyph, FT_GlyphSlot slot, FT_BBox* boundingBox);

	/**
	 * Destroys the glyph and its data buffer
	 */
	~g_glyph();

	/**
	 *
	 */
	g_color_argb* getBitmap() {
		return content;
	}

	/**
	 *
	 */
	g_dimension getBitmapSize() {
		return contentSize;
	}

	/**
	 *
	 */
	g_rectangle getBounding() {
		return bounding;
	}

	/**
	 *
	 */
	g_point getAdvance();

	/**
	 *
	 */
	g_point getBitmapTopLeftPixel() {
		return bitmapTopLeftPixel;
	}

	/**
	 *
	 */
	FT_UInt getIndex() {
		return glyphIndex;
	}

};

#endif
