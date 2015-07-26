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

#include <ghostuser/graphics/text/glyph.hpp>
#include <ghostuser/utils/logger.hpp>

/**
 *
 */
g_glyph::g_glyph(FT_UInt glyphIndex, FT_Glyph glyph, FT_GlyphSlot slot, FT_BBox* boundingBox) :
		glyphIndex(glyphIndex), glyph(glyph), content(0) {

	copyBitmap(&slot->bitmap);

	bitmapTopLeftPixel = g_point(slot->bitmap_left, slot->bitmap_top);

	advance.x = slot->advance.x >> 6;
	advance.y = slot->advance.y >> 6;

	bounding.x = boundingBox->xMin;
	bounding.y = boundingBox->yMin;
	bounding.width = boundingBox->xMax - boundingBox->xMin;
	bounding.height = boundingBox->yMax - boundingBox->yMin;
}

/**
 *
 */
g_glyph::~g_glyph() {
	if (content) {
		delete content;
	}
}

/**
 *
 */
void g_glyph::copyBitmap(FT_Bitmap* bitmap) {

	content = new g_color_argb[bitmap->rows * bitmap->width];
	contentSize.width = bitmap->width;
	contentSize.height = bitmap->rows;

	if (bitmap->pixel_mode == FT_PIXEL_MODE_BGRA) {
		for (int py = 0; py < bitmap->rows; py++) {
			for (int px = 0; px < bitmap->width; px += 3) {
				uint8_t cb = bitmap->buffer[py * bitmap->pitch + px];
				uint8_t cg = bitmap->buffer[py * bitmap->pitch + px + 1];
				uint8_t cr = bitmap->buffer[py * bitmap->pitch + px + 2];
				uint8_t ca = bitmap->buffer[py * bitmap->pitch + px + 3];

				content[py * bitmap->width + px] = (ca << 24) | (cr << 16) | (cg << 8) | cb;
			}
		}

	} else if (bitmap->pixel_mode == FT_PIXEL_MODE_GRAY) {
		for (int py = 0; py < bitmap->rows; py++) {
			for (int px = 0; px < bitmap->width; px++) {
				uint8_t intensity = bitmap->buffer[py * bitmap->pitch + px];
				content[py * bitmap->width + px] = (intensity << 24);
			}
		}

	} else if (bitmap->pixel_mode == FT_PIXEL_MODE_LCD) {
		for (int py = 0; py < bitmap->rows; py++) {
			for (int px = 0; px < bitmap->width; px += 3) {
				uint8_t intensity1 = bitmap->buffer[py * bitmap->pitch + px];
				uint8_t intensity2 = bitmap->buffer[py * bitmap->pitch + px + 1];
				uint8_t intensity3 = bitmap->buffer[py * bitmap->pitch + px + 2];

				uint8_t intensity = (intensity1 + intensity2 + intensity3) / 3;

				content[py * bitmap->width + px / 3] = (intensity << 24);
			}
		}

	} else if (bitmap->pixel_mode == FT_PIXEL_MODE_MONO) {
		for (int py = 0; py < bitmap->rows; py++) {
			for (int px = 0; px < bitmap->width; px++) {

				uint8_t* row = &bitmap->buffer[bitmap->pitch * py];
				uint8_t byte = row[px >> 3];
				uint8_t intensity = ((byte & (128 >> (px & 7))) != 0) ? 0xFF : 0;

				content[py * bitmap->width + px] = (intensity << 24) | ((0x00 << 16) & 0xFF) | ((0x00 << 8) & 0xFF) | (0x00 & 0xFF);
			}
		}

	}
}

/**
 *
 */
g_point g_glyph::getAdvance() {
	return advance;
}
