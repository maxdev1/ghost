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

#include <ghostuser/graphics/images/ghost_image_format.hpp>
#include <ghostuser/graphics/images/image.hpp>
#include <ghostuser/io/files/file_utils.hpp>
#include <cstring>

static const char* SIGNATURE = "GHOST";

/**
 *
 */
std::string g_ghost_image_format::getName() {
	return "Ghost Image";
}

/**
 *
 */
g_image_decoding_status g_ghost_image_format::decode(FILE* input, g_image& image) {
	uint32_t offset = 0;

	// Read signature
	uint32_t signatureLength = std::strlen(SIGNATURE);
	uint8_t signatureBuffer[signatureLength];
	if (!g_file_utils::tryReadBytes(input, offset, signatureBuffer, signatureLength)) {
		return g_image_decoding_status::FAILED;
	}

	bool signatureValid = true;
	for (uint32_t i = 0; i < signatureLength; i++) {
		if (signatureBuffer[i] != SIGNATURE[i]) {
			signatureValid = false;
		}
	}

	if (!signatureValid) {
		return g_image_decoding_status::FAILED;
	}
	offset += signatureLength;

	// Read width and height
	uint32_t whLength = 4;
	uint8_t whBuffer[whLength];
	if (!g_file_utils::tryReadBytes(input, offset, whBuffer, whLength)) {
		return g_image_decoding_status::FAILED;
	}

	image.width = (whBuffer[0] << 8) | whBuffer[1];
	image.height = (whBuffer[2] << 8) | whBuffer[3];
	uint32_t imagePixelCount = image.width * image.height;
	uint32_t imageByteCount = imagePixelCount * 4;
	offset += whLength;

	// Read content and convert to ARGB-pixels
	image.buffer = new uint32_t[imagePixelCount];

	uint8_t* temp = new uint8_t[imageByteCount];
	if (temp == 0) {
		return g_image_decoding_status::FAILED;
	}
	if (!g_file_utils::tryReadBytes(input, offset, temp, imageByteCount)) {
		delete temp;
		return g_image_decoding_status::FAILED;
	}

	for (int i = 0; i < imagePixelCount; i++) {
		image.buffer[i] = (temp[i * 4] << 24) | (temp[i * 4 + 1] << 16) | (temp[i * 4 + 2] << 8) | (temp[i * 4 + 3]);
	}
	delete temp;
	return g_image_decoding_status::SUCCESSFUL;
}

/**
 *
 */
g_image_encoding_status g_ghost_image_format::encode(g_image& image, FILE* output) {
	return g_image_encoding_status::FAILED;
}

