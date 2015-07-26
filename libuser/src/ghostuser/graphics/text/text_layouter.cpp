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

#include <ghostuser/graphics/text/text_layouter.hpp>
#include <ghostuser/utils/logger.hpp>

static g_text_layouter* instance = 0;

/**
 * @see header
 */
g_text_layouter* g_text_layouter::getInstance() {
	if (instance == 0) {
		instance = new g_text_layouter();
	}
	return instance;
}

/**
 *
 */
void rightAlign(g_layouted_text& text, int line, int lineWidth, g_rectangle& bounds) {
	int difference = bounds.width - bounds.x - lineWidth;
	for (g_positioned_glyph& g : text.positions) {
		if (g.line == line) {
			g.position.x += difference;
		}
	}
}

/**
 *
 */
void centerAlign(g_layouted_text& text, int line, int lineWidth, g_rectangle& bounds) {
	int difference = (bounds.width - bounds.x) / 2 - lineWidth / 2;
	for (g_positioned_glyph& g : text.positions) {
		if (g.line == line) {
			g.position.x += difference;
		}
	}
}

/**
 *
 */
void g_text_layouter::layout(std::string text, g_font* font, int size, g_rectangle bounds, g_text_alignment alignment, g_layouted_text& out,
		bool breakOnOverflow) {

	if (font == 0) {
		return;
	}

	int x = bounds.x;
	int y = bounds.y;
	int lineStartX = x;

	int line = 0;
	int lineHeight = font->getLineHeight(size);

	g_glyph* previous = 0;
	for (int i = 0; i < text.length(); i++) {
		char chr = text[i];

		bool invisible = false;

		g_glyph* glyph = font->getGlyph(size, chr);
		if (glyph != 0) {
			// Create new position
			g_positioned_glyph positioned;
			positioned.glyph = glyph;

			// Add kerning
			if (previous) {
				x += font->getKerning(size, previous, glyph).x;
			}

			// Wouldn't match in line or is break character? Start next line
			if (chr == '\n' || (breakOnOverflow && (x + glyph->getBounding().width > bounds.width))) {
				if (chr == '\n') {
					invisible = true;
				}

				if (alignment == g_text_alignment::RIGHT) {
					rightAlign(out, line, x - lineStartX, bounds);
				} else if (alignment == g_text_alignment::CENTER) {
					centerAlign(out, line, x - lineStartX, bounds);
				}

				++line;
				x = bounds.x;
				lineStartX = x;
				y += lineHeight;
			}

			if (!invisible) {
				// Position
				positioned.line = line;
				positioned.position.x = x + glyph->getBitmapTopLeftPixel().x;
				positioned.position.y = y + lineHeight - glyph->getBitmapTopLeftPixel().y;

				// Add position
				out.positions.push_back(positioned);

				// Jump to next
				x += glyph->getAdvance().x;
			}
			previous = glyph;
		}
	}

	if (alignment == g_text_alignment::RIGHT) {
		rightAlign(out, line, x - lineStartX, bounds);
	} else if (alignment == g_text_alignment::CENTER) {
		centerAlign(out, line, x - lineStartX, bounds);
	}

	// Set text bounds
#define BOUNDS_EMPTY 0xFFFFFF
	int tbTop = BOUNDS_EMPTY;
	int tbLeft = BOUNDS_EMPTY;
	int tbRight = 0;
	int tbBottom = 0;

	for (g_positioned_glyph& p : out.positions) {
		if (p.position.x < tbLeft) {
			tbLeft = p.position.x;
		}
		if (p.position.y < tbTop) {
			tbTop = p.position.y;
		}
		int r = p.position.x + p.glyph->getBitmapTopLeftPixel().x + p.glyph->getBitmapSize().width;
		if (r > tbRight) {
			tbRight = r;
		}
		int b = p.position.y + p.glyph->getBitmapTopLeftPixel().y + p.glyph->getBitmapSize().height;
		if (b > tbBottom) {
			tbBottom = b;
		}
	}

	if (tbTop != BOUNDS_EMPTY && tbLeft != BOUNDS_EMPTY) {
		out.textBounds.x = tbLeft;
		out.textBounds.y = tbTop;
		out.textBounds.width = tbRight - tbLeft;
		out.textBounds.height = tbBottom - tbTop;
	}
}

