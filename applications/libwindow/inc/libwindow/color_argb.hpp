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

#ifndef LIBWINDOW_COLORARGB
#define LIBWINDOW_COLORARGB

#include <cstdint>

#define ARGB(a, r, g, b) ((a << 24) | (r << 16) | (g << 8) | (b))
#define RGB(r, g, b) ARGB(0xFF, r, g, b)

#define ARGB_A_FROM(argb) ((argb >> 24) & 0xFF)
#define ARGB_R_FROM(argb) ((argb >> 16) & 0xFF)
#define ARGB_G_FROM(argb) ((argb >> 8) & 0xFF)
#define ARGB_B_FROM(argb) ((argb >> 0) & 0xFF)

#define ARGB_FA_FROM(argb) ((double) ((argb >> 24) & 0xFF) / 255)
#define ARGB_FR_FROM(argb) ((double) ((argb >> 16) & 0xFF) / 255)
#define ARGB_FG_FROM(argb) ((double) ((argb >> 8) & 0xFF) / 255)
#define ARGB_FB_FROM(argb) ((double) ((argb >> 0) & 0xFF) / 255)

#define G_COLOR_ARGB_TO_FPARAMS(argb) ARGB_FR_FROM(argb), ARGB_FG_FROM(argb), ARGB_FB_FROM(argb), ARGB_FA_FROM(argb)

typedef uint32_t g_color_argb;

#endif
