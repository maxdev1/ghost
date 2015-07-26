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

#include <ghostuser/graphics/painter.hpp>
#include <cstdint>
#include <cmath>
#include <cstring>

/**
 *
 */
void g_painter::fill(g_rectangle r) {
	for (int y = r.y; y < r.y + r.height; y++) {
		for (int x = r.x; x < r.x + r.width; x++) {
			graphics.paintPixel(x, y, color);
		}
	}
}

/**
 *
 */
void g_painter::draw(g_rectangle r) {
	for (int y = r.y; y < r.y + r.height; y++) {
		graphics.paintPixel(0, y, color);
	}
	for (int y = r.y; y < r.y + r.height; y++) {
		graphics.paintPixel(r.width, y, color);
	}
	for (int x = r.x; x < r.x + r.width; x++) {
		graphics.paintPixel(x, 0, color);
	}
	for (int x = r.x; x <= r.x + r.width; x++) {
		graphics.paintPixel(x, r.height, color);
	}
}

/**
 *
 */
void g_painter::drawImage(int px, int py, g_image* image) {

	g_color_argb* img = image->getContent();
	if (img) {
		drawBitmap(px, py, img, image->getWidth(), image->getHeight());
	}
}

/**
 *
 */
void g_painter::drawBitmap(int px, int py, g_color_argb* bitmap, int width, int height) {
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			graphics.paintPixel(px + x, py + y, bitmap[y * width + x]);
		}
	}
}

/**
 *
 */
void g_painter::drawColoredBitmap(int px, int py, g_color_argb* bitmap, g_color_argb color, int width, int height) {
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			g_color_argb bitmapColor = bitmap[y * width + x];
			bitmapColor = (bitmapColor & ~0xFF0000) | (color & 0xFF0000);
			bitmapColor = (bitmapColor & ~0xFF00) | (color & 0xFF00);
			bitmapColor = (bitmapColor & ~0xFF) | (color & 0xFF);
			graphics.paintPixel(px + x, py + y, bitmapColor);
		}
	}
}

/**
 *
 */
void g_painter::fill(g_polygon& p) {

	auto points = p.getPoints();
	g_rectangle polygonBounds = p.getBounds();
	int polygonLeft = polygonBounds.getLeft();
	int polygonRight = polygonBounds.getRight();

	int nodeX[points.size()];

	for (int pixelY = polygonBounds.getTop(); pixelY < polygonBounds.getBottom(); pixelY++) {

		// Find X points that collide with horizontal line at Y
		int nodes = 0;
		int j = points.size() - 1;
		for (int i = 0; i < points.size(); i++) {
			double x1 = points[i].x;
			double y1 = points[i].y;
			double x2 = points[j].x;
			double y2 = points[j].y;

			if (((y1 < (double) pixelY) && (y2 >= (double) pixelY)) || ((y2 < (double) pixelY && y1 >= (double) pixelY))) {
				nodeX[nodes++] = (int) (x1 + (pixelY - y1) / (y2 - y1) * (x2 - x1));
			}

			j = i;
		}

		// Sort X ascending
		int i = 0;
		while (i < nodes - 1) {
			if (nodeX[i] > nodeX[i + 1]) {
				int swap = nodeX[i];
				nodeX[i] = nodeX[i + 1];
				nodeX[i + 1] = swap;
				if (i) {
					i--;
				}
			} else {
				i++;
			}
		}

		// Fill between Xs
		for (int i = 0; i < nodes; i += 2) {
			if (nodeX[i] >= polygonRight)
				break;
			if (nodeX[i + 1] > polygonLeft) {
				if (nodeX[i] < polygonLeft) {
					nodeX[i] = polygonLeft;
				}
				if (nodeX[i + 1] > polygonRight) {
					nodeX[i + 1] = polygonRight;
				}
				for (j = nodeX[i]; j < nodeX[i + 1]; j++) {
					graphics.putPixel(j, pixelY, color);
				}
			}
		}
	}

}

/**
 *
 */
void g_painter::blur(double intensity) {
	for (int y = 0; y < graphics.getHeight(); y++) {
		for (int x = 0; x < graphics.getWidth(); x++) {
			int totalA = 0;
			int totalR = 0;
			int totalG = 0;
			int totalB = 0;
			for (int sy = -(intensity / 2); sy < intensity / 2; sy++) {
				for (int sx = -(intensity / 2); sx < intensity / 2; sx++) {
					g_color_argb c = graphics.getPixel(x + sx, y + sy);
					totalA += ARGB_A_FROM(c);
					totalR += ARGB_R_FROM(c);
					totalG += ARGB_G_FROM(c);
					totalB += ARGB_B_FROM(c);
				}
			}

			totalA /= intensity * intensity;
			totalR /= intensity * intensity;
			totalG /= intensity * intensity;
			totalB /= intensity * intensity;

			graphics.putPixel(x, y, ARGB(totalA, totalR, totalG, totalB));
		}
	}
}

double fpart(double x) {
	return x - ((long) x);
}

double rfpart(double x) {
	return 1 - fpart(x);
}

/**
 *
 */
void g_painter::drawLine(g_point a, g_point b) {

	double x0 = a.x;
	double y0 = a.y;
	double x1 = b.x;
	double y1 = b.y;

	double alpha = ARGB_A_FROM(color);
	g_color_argb rgb = 0xFFFFFF & color;

	bool steep = std::abs(y1 - y0) > std::abs(x1 - x0);

	if (steep) {
		double s = y0;
		y0 = x0;
		x0 = s;

		s = y1;
		y1 = x1;
		x1 = s;
	}
	if (x0 > x1) {
		double s = x1;
		x1 = x0;
		x0 = s;

		s = y1;
		y1 = y0;
		y0 = s;
	}

	double dx = x1 - x0;
	double dy = y1 - y0;
	double gradient = dy / dx;

	// handle first endpoint
	double xend = round(x0);
	double yend = y0 + gradient * (xend - x0);
	double xgap = rfpart(x0 + 0.5);
	double xpxl1 = xend;
	double ypxl1 = (long) (yend);
	if (steep) {
		graphics.paintPixel(ypxl1, xpxl1, ((g_color_argb) (rfpart(yend) * xgap * alpha)) << 24 | rgb);
		graphics.paintPixel(ypxl1 + 1, xpxl1, ((g_color_argb) (fpart(yend) * xgap * alpha)) << 24 | rgb);
	} else {
		graphics.paintPixel(xpxl1, ypxl1, ((g_color_argb) (rfpart(yend) * xgap * alpha)) << 24 | rgb);
		graphics.paintPixel(xpxl1, ypxl1 + 1, ((g_color_argb) (fpart(yend) * xgap * alpha)) << 24 | rgb);
	}
	double intery = yend + gradient; // first y-intersection for the main loop

	// handle second endpoint
	xend = round(x1);
	yend = y1 + gradient * (xend - x1);
	xgap = fpart(x1 + 0.5);
	double xpxl2 = xend; //this will be used in the main loop
	double ypxl2 = (long) (yend);
	if (steep) {
		graphics.paintPixel(ypxl2, xpxl2, ((g_color_argb) (rfpart(yend) * xgap * alpha)) << 24 | rgb);
		graphics.paintPixel(ypxl2 + 1, xpxl2, ((g_color_argb) (fpart(yend) * xgap * alpha)) << 24 | rgb);
	} else {
		graphics.paintPixel(xpxl2, ypxl2, ((g_color_argb) (rfpart(yend) * xgap * alpha)) << 24 | rgb);
		graphics.paintPixel(xpxl2, ypxl2 + 1, ((g_color_argb) (fpart(yend) * xgap * alpha)) << 24 | rgb);
	}

	// main loop

	for (double x = xpxl1 + 1; x <= xpxl2 - 1; x++) {
		if (steep) {
			graphics.paintPixel((long) (intery), x, ((g_color_argb) (rfpart(intery) * alpha)) << 24 | rgb);
			graphics.paintPixel((long) (intery) + 1, x, ((g_color_argb) (fpart(intery) * alpha)) << 24 | rgb);
		} else {
			graphics.paintPixel(x, (long) (intery), ((g_color_argb) (rfpart(intery) * alpha)) << 24 | rgb);
			graphics.paintPixel(x, (long) (intery) + 1, ((g_color_argb) (fpart(intery) * alpha)) << 24 | rgb);
		}
		intery = intery + gradient;
	}
}

