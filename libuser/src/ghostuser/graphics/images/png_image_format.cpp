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

#include <ghostuser/graphics/images/png_image_format.hpp>
#include <ghostuser/graphics/images/image.hpp>
#include <ghostuser/io/files/file_utils.hpp>
#include <ghost/utils/local.hpp>
#include <cstring>
#include <stdlib.h>

#include <libpng16/png.h>

/**
 *
 */
std::string g_png_image_format::getName() {
	return "Portable Network Graphics";
}

/**
 *
 */
g_image_decoding_status g_png_image_format::decode(FILE* fp, g_image& image) {

	// read PNG header
	uint8_t header[8];
	fread(header, 1, 8, fp);

	// check signature
	if (png_sig_cmp(header, 0, 8)) {
		klog("error: given file is not a PNG");
		return g_image_decoding_status::FAILED;
	}

	// create read structure
	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr) {
		klog("error: png_create_read_struct failed");
		return g_image_decoding_status::FAILED;
	}

	// create png info struct
	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		klog("error: png_create_info_struct failed");
		png_destroy_read_struct(&png_ptr, (png_infopp) NULL, (png_infopp) NULL);
		return g_image_decoding_status::FAILED;
	}

	// create png info struct
	png_infop end_info = png_create_info_struct(png_ptr);
	if (!end_info) {
		klog("error: png_create_info_struct failed");
		png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
		return g_image_decoding_status::FAILED;
	}

	// the code in this if statement gets called if libpng encounters an error
	if (setjmp(png_jmpbuf(png_ptr))) {
		klog("error: encountered a problem within libpng");
		png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
		return g_image_decoding_status::FAILED;
	}

	// init png reading
	png_init_io(png_ptr, fp);

	// we already read the first 8 bytes
	png_set_sig_bytes(png_ptr, 8);

	// read information
	png_read_info(png_ptr, info_ptr);

	// get image information
	image.width = png_get_image_width(png_ptr, info_ptr);
	image.height = png_get_image_height(png_ptr, info_ptr);
	png_byte color_type = png_get_color_type(png_ptr, info_ptr);
	png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);

	// update png info
	int number_of_passes = png_set_interlace_handling(png_ptr);
	png_read_update_info(png_ptr, info_ptr);

	// check bit depth
	if (bit_depth != 8) {
		klog("error: only 8 bit images supported");
		png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
		return g_image_decoding_status::FAILED;
	}

	// check color type
	if (!(color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_RGBA || color_type == PNG_COLOR_TYPE_RGB_ALPHA)) {
		klog("error: only 8 bit images supported");
		png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
		return g_image_decoding_status::FAILED;
	}

	// allocate memory
	png_size_t bytes_per_row = png_get_rowbytes(png_ptr, info_ptr);

	g_local<png_bytep> row_pointers_local((png_bytep*) malloc(sizeof(png_bytep) * bytes_per_row * image.height));
	png_bytep* row_pointers = row_pointers_local();

	if (row_pointers == nullptr) {
		klog("error: failed to allocate memory");
		png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
		return g_image_decoding_status::FAILED;
	}

	// read image contents
	png_read_image(png_ptr, row_pointers);

	// convert to image
	image.buffer = new g_color_argb[image.width * image.height];

	for (int py = 0; py < image.height; py++) {
		for (int px = 0; px < image.width; px++) {
			image.buffer[py * image.width + px] = ARGB(row_pointers[py][px * 4 + 3], row_pointers[py][px * 4], row_pointers[py][px * 4 + 1],
					row_pointers[py][px * 4 + 2]);
		}
	}

	// clean up
	png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);

	return g_image_decoding_status::SUCCESSFUL;
}

/**
 *
 */
g_image_encoding_status g_png_image_format::encode(g_image& image, FILE* output) {
	return g_image_encoding_status::FAILED;
}

