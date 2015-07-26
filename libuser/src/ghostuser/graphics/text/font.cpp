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

#include <ghostuser/graphics/text/font.hpp>
#include <ghostuser/graphics/text/font_manager.hpp>
#include <ghostuser/io/files/file_utils.hpp>
#include <ghostuser/utils/logger.hpp>
#include <string.h>

/**
 *
 */
g_font::g_font(std::string name, uint8_t* source, uint32_t sourceLength, g_font_style style, bool hint) :
		name(name), data(0), face(0), okay(false), style(style), activeSize(0), hint(hint) {

	data = new uint8_t[sourceLength];
	memcpy(data, source, sourceLength);

	// Check data
	if (data == 0) {
		g_logger::log("failed to allocate memory buffer for font '" + name + "'");
		return;
	}

	// Load font
	FT_Error error = FT_New_Memory_Face(g_font_manager::getInstance()->getLibraryHandle(), data, sourceLength, 0, &face);
	if (error) {
		g_logger::log("freetype2 failed at FT_New_Memory_Face with error code %i", error);

		delete data;
		return;
	}

	// TODO apply style

	okay = true;
}

/**
 *
 */
g_font::~g_font() {
	if (okay) {
		// Finish the sizes
		for (auto size : sizes) {
			FT_Done_Size(size.second);
		}

		// Destroy the glyphs
		destroyGlyphs();

		// Destroy the font
		FT_Done_Face(face);

		delete data;
	}

}

/**
 *
 */
g_font* g_font::fromFile(FILE* in, std::string name) {

	g_font* existing = g_font_manager::getInstance()->getFont(name);
	if (existing) {
		return existing;
	}

	uint32_t length = g_length(fileno(in));
	if (length == -1 || length > 1024 * 1024 * 5) {
		return 0;
	}

	uint8_t* fileContent = new uint8_t[length];
	if (!g_file_utils::tryReadBytes(in, 0, fileContent, length)) {
		delete fileContent;
		return 0;
	}

	bool created = g_font_manager::getInstance()->createFont(name, fileContent, length);
	if (!created) {
		delete fileContent;
		return 0;
	}

	return g_font_manager::getInstance()->getFont(name);
}

/**
 *
 */
void g_font::destroyGlyphs() {
	for (auto sizeEntry : glyphCache) {
		for (auto glyphEntry : sizeEntry.second) {
			delete glyphEntry.second;
		}
	}
}

/**
 *
 */
bool g_font::isOkay() {
	return okay;
}

/**
 *
 */
FT_Size g_font::getSize(int size) {
	if (sizes.count(size) > 0) {
		return sizes[size];
	}

	// Create the size
	FT_Size newSize;
	FT_Error error = FT_New_Size(face, &newSize);
	if (error) {
		g_logger::log("freetype2 failed at FT_New_Size with error code %i", error);
		return 0;
	}

	// Activate & scale the size
	error = FT_Activate_Size(newSize);
	if (error) {
		g_logger::log("freetype2 failed at FT_Activate_Size with error code %i", error);
		return 0;
	}
	activeSize = size;

	error = FT_Set_Char_Size(face, 0, size * 64, 96, 0);
	if (error) {
		g_logger::log("freetype2 failed at FT_Set_Char_Size with error code %i", error);
		return 0;
	}

	sizes[size] = newSize;
	return newSize;
}

/**
 *
 */
g_glyph* g_font::getGlyph(int size, char c) {

	// Check the cache for existing size
	if (glyphCache.count(size) > 0) {
		g_font_glyph_map& glyphMap = glyphCache.at(size);

		// ... and existing glyph
		if (glyphMap.count(c)) {

			// return the glyph
			return glyphMap.at(c);
		}
	}

	FT_Error error;

	// Set the size
	if (activeSize != size) {
		error = FT_Activate_Size(getSize(size));
		if (error) {
			g_logger::log("freetype2 failed at FT_Activate_Size with error code %i", error);
			return 0;
		}
		activeSize = size;
	}

	// Render the glyph
	FT_UInt glyphIndex = FT_Get_Char_Index(face, c);

	error = FT_Load_Glyph(face, glyphIndex, FT_LOAD_DEFAULT);
	if (error) {
		g_logger::log("freetype2 failed at FT_Load_Glyph with error code %i", error);
		return 0;
	}

	error = FT_Render_Glyph(face->glyph, hint ? FT_RENDER_MODE_NORMAL : FT_RENDER_MODE_MONO);
	if (error) {
		g_logger::log("freetype2 failed at FT_Render_Glyph with error code %i", error);
		return 0;
	}

	FT_Glyph glyph;
	error = FT_Get_Glyph(face->glyph, &glyph);
	if (error) {
		g_logger::log("freetype2 failed at FT_Get_Glyph with error code %i", error);
		return 0;
	}

	// Get bounding box
	FT_BBox bbox;
	FT_Glyph_Get_CBox(glyph, ft_glyph_bbox_pixels, &bbox);

	// Create the glyph
	g_glyph* glyphObj = new g_glyph(glyphIndex, glyph, face->glyph, &bbox);
	glyphCache[size][c] = glyphObj;
	return glyphObj;
}

/**
 *
 */
int g_font::getLineHeight(int size) {

	// Set the size
	if (activeSize != size) {
		FT_Error error = FT_Activate_Size(getSize(size));
		if (error) {
			g_logger::log("freetype2 failed at FT_Activate_Size with error code %i", error);
			return 0;
		}
		activeSize = size;
	}

	return face->size->metrics.height >> 6;
}

/**
 *
 */
g_point g_font::getKerning(int size, g_glyph* left, g_glyph* right) {

	if (FT_HAS_KERNING(face)) {
		FT_Error error;

		// Set the size
		if (activeSize != size) {
			error = FT_Activate_Size(getSize(size));
			if (error) {
				g_logger::log("freetype2 failed at FT_Activate_Size with error code %i", error);
				return g_point();
			}
			activeSize = size;
		}

		// Get the kerning
		FT_Vector delta;
		error = FT_Get_Kerning(face, left->getIndex(), right->getIndex(), FT_KERNING_DEFAULT, &delta);
		if (error) {
			g_logger::log("freetype2 failed at FT_Get_Kerning with error code %i", error);
		} else {
			return g_point(delta.x >> 6, delta.y >> 6);
		}
	}

	return g_point();
}

