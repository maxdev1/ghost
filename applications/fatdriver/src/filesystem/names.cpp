/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2026, Max Schlüssel <lokoxe@gmail.com>                     *
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

#include <cstring>
#include "fatdriver.hpp"
#include "unicode_uppercase.hpp"

static uint32_t fatNameUpper(uint32_t value)
{
	size_t first = 0, end = sizeof(fatCaseRanges) / sizeof(fatCaseRanges[0]);
	while(first < end)
	{
		auto middle = first + (end - first) / 2;
		auto& range = fatCaseRanges[middle];
		if(value < range.first)
			end = middle;
		else if(value > range.last)
			first = middle + 1;
		else
			return (value - range.first) % range.step ? value : uint32_t(int64_t(value) + range.delta);
	}
	return value;
}

static bool fatNameReadUtf8(const char** text, size_t* remaining, uint32_t* value)
{
	if(!*remaining)
		return false;

	auto bytes = reinterpret_cast<const uint8_t*>(*text);
	uint32_t scalar = bytes[0];
	unsigned count = scalar < 0x80						? 1
					 : scalar >= 0xc2 && scalar <= 0xdf ? 2
					 : scalar >= 0xe0 && scalar <= 0xef ? 3
					 : scalar >= 0xf0 && scalar <= 0xf4 ? 4
														: 0;
	if(!count || count > *remaining)
		return false;
	if(count > 1)
		scalar &= (1U << (7 - count)) - 1;
	for(unsigned i = 1; i < count; ++i)
	{
		if((bytes[i] & 0xc0) != 0x80)
			return false;
		scalar = (scalar << 6) | (bytes[i] & 0x3f);
	}
	if((count == 2 && scalar < 0x80) || (count == 3 && scalar < 0x800) || (count == 4 && scalar < 0x10000) ||
	   scalar > 0x10ffff || (scalar >= 0xd800 && scalar <= 0xdfff))
		return false;

	*text += count;
	*remaining -= count;
	*value = scalar;
	return true;
}

bool fatNameEquals(const char* left, const char* right)
{
	size_t leftRemaining = G_FILENAME_MAX, rightRemaining = G_FILENAME_MAX;
	for(;;)
	{
		uint32_t a, b;
		if(!fatNameReadUtf8(&left, &leftRemaining, &a) || !fatNameReadUtf8(&right, &rightRemaining, &b) ||
		   fatNameUpper(a) != fatNameUpper(b))
			return false;
		if(!a)
			return true;
	}
}

static bool fatNameAppendUtf8(char* name, size_t* length, size_t capacity, uint32_t value)
{
	if(!value || value < 0x20 || value == '/' || value == '\\' || value == ':' || value == '*' || value == '?' ||
	   value == '"' || value == '<' || value == '>' || value == '|')
		return false;

	unsigned count = value < 0x80 ? 1 : value < 0x800 ? 2 : value < 0x10000 ? 3 : 4;
	if(*length + count >= capacity)
		return false;
	if(count == 1)
		name[(*length)++] = value;
	else
	{
		name[(*length)++] = (0xff << (8 - count)) | (value >> (6 * (count - 1)));
		for(unsigned i = count - 1; i > 0; --i)
			name[(*length)++] = 0x80 | ((value >> (6 * (i - 1))) & 0x3f);
	}
	name[*length] = 0;
	return true;
}

static const uint16_t fatOemCharacters[128] = {
		0x00c7, 0x00fc, 0x00e9, 0x00e2, 0x00e4, 0x00e0, 0x00e5, 0x00e7, 0x00ea, 0x00eb, 0x00e8, 0x00ef, 0x00ee,
		0x00ec, 0x00c4, 0x00c5, 0x00c9, 0x00e6, 0x00c6, 0x00f4, 0x00f6, 0x00f2, 0x00fb, 0x00f9, 0x00ff, 0x00d6,
		0x00dc, 0x00a2, 0x00a3, 0x00a5, 0x20a7, 0x0192, 0x00e1, 0x00ed, 0x00f3, 0x00fa, 0x00f1, 0x00d1, 0x00aa,
		0x00ba, 0x00bf, 0x2310, 0x00ac, 0x00bd, 0x00bc, 0x00a1, 0x00ab, 0x00bb, 0x2591, 0x2592, 0x2593, 0x2502,
		0x2524, 0x2561, 0x2562, 0x2556, 0x2555, 0x2563, 0x2551, 0x2557, 0x255d, 0x255c, 0x255b, 0x2510, 0x2514,
		0x2534, 0x252c, 0x251c, 0x2500, 0x253c, 0x255e, 0x255f, 0x255a, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550,
		0x256c, 0x2567, 0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256b, 0x256a, 0x2518, 0x250c,
		0x2588, 0x2584, 0x258c, 0x2590, 0x2580, 0x03b1, 0x00df, 0x0393, 0x03c0, 0x03a3, 0x03c3, 0x00b5, 0x03c4,
		0x03a6, 0x0398, 0x03a9, 0x03b4, 0x221e, 0x03c6, 0x03b5, 0x2229, 0x2261, 0x00b1, 0x2265, 0x2264, 0x2320,
		0x2321, 0x00f7, 0x2248, 0x00b0, 0x2219, 0x00b7, 0x221a, 0x207f, 0x00b2, 0x25a0, 0x00a0,
};

static const uint16_t fatOemLowerCharacters[128] = {
		0x00e7, 0x00fc, 0x00e9, 0x00e2, 0x00e4, 0x00e0, 0x00e5, 0x00e7, 0x00ea, 0x00eb, 0x00e8, 0x00ef, 0x00ee,
		0x00ec, 0x00e4, 0x00e5, 0x00e9, 0x00e6, 0x00e6, 0x00f4, 0x00f6, 0x00f2, 0x00fb, 0x00f9, 0x00ff, 0x00f6,
		0x00fc, 0x00a2, 0x00a3, 0x00a5, 0x20a7, 0x0192, 0x00e1, 0x00ed, 0x00f3, 0x00fa, 0x00f1, 0x00f1, 0x00aa,
		0x00ba, 0x00bf, 0x2310, 0x00ac, 0x00bd, 0x00bc, 0x00a1, 0x00ab, 0x00bb, 0x2591, 0x2592, 0x2593, 0x2502,
		0x2524, 0x2561, 0x2562, 0x2556, 0x2555, 0x2563, 0x2551, 0x2557, 0x255d, 0x255c, 0x255b, 0x2510, 0x2514,
		0x2534, 0x252c, 0x251c, 0x2500, 0x253c, 0x255e, 0x255f, 0x255a, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550,
		0x256c, 0x2567, 0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256b, 0x256a, 0x2518, 0x250c,
		0x2588, 0x2584, 0x258c, 0x2590, 0x2580, 0x03b1, 0x00df, 0x03b3, 0x03c0, 0x03c3, 0x03c3, 0x00b5, 0x03c4,
		0x03c6, 0x03b8, 0x03c9, 0x03b4, 0x221e, 0x03c6, 0x03b5, 0x2229, 0x2261, 0x00b1, 0x2265, 0x2264, 0x2320,
		0x2321, 0x00f7, 0x2248, 0x00b0, 0x2219, 0x00b7, 0x221a, 0x207f, 0x00b2, 0x25a0, 0x00a0,
};

static bool fatShortNamePart(const uint8_t* raw, unsigned count, bool lower, char* name, size_t* length)
{
	while(count && raw[count - 1] == ' ')
		--count;
	for(unsigned i = 0; i < count; ++i)
	{
		auto c = raw[i];
		uint32_t scalar = c < 0x80 ? c : lower ? fatOemLowerCharacters[c - 0x80] : fatOemCharacters[c - 0x80];
		if(lower && scalar >= 'A' && scalar <= 'Z')
			scalar += 'a' - 'A';
		if(!fatNameAppendUtf8(name, length, 40, scalar))
			return false;
	}
	return true;
}

bool fatShortNameDecode(const uint8_t* raw, char* name)
{
	uint8_t base[8];
	memcpy(base, raw, sizeof(base));
	if(base[0] == 0x05)
		base[0] = 0xe5;

	size_t length = 0;
	if(!fatShortNamePart(base, 8, raw[12] & 0x08, name, &length) || !length)
		return false;
	if(raw[8] != ' ')
	{
		name[length++] = '.';
		if(!fatShortNamePart(raw + 8, 3, raw[12] & 0x10, name, &length))
			return false;
	}
	name[length] = 0;
	return true;
}

void fatLongNameAppend(g_fat_long_name* name, const uint8_t* raw)
{
	auto ordinal = unsigned(raw[0] & 0x1f);
	if((raw[0] & 0xa0) || !ordinal || ordinal > G_FAT_LONG_NAME_MAX_ENTRIES || raw[12] || fatRead16(raw + 26))
	{
		name->active = false;
		return;
	}
	if(raw[0] & 0x40)
	{
		*name = {};
		name->active = true;
		name->expected = ordinal;
		name->checksum = raw[13];
		name->length = ordinal * G_FAT_LONG_NAME_UNITS_PER_ENTRY;
	}
	if(!name->active || ordinal != name->expected || raw[13] != name->checksum)
	{
		name->active = false;
		return;
	}

	for(unsigned i = 0; i < G_FAT_LONG_NAME_UNITS_PER_ENTRY; ++i)
		name->units[(ordinal - 1) * G_FAT_LONG_NAME_UNITS_PER_ENTRY + i] = fatRead16(raw + G_FAT_LONG_NAME_OFFSETS[i]);
	--name->expected;
}

/**
 * Invalid or orphaned long-name runs fall back to their short alias. Validate the
 * sequence, checksum, UTF-16 and padding before publishing any UTF-8 filename.
 */
bool fatLongNameDecode(const g_fat_long_name& name, const uint8_t* raw, char* text)
{
	if(!name.active || name.expected)
		return false;
	auto checksum = fatShortNameChecksum(raw);
	if(checksum != name.checksum)
		return false;

	size_t length = 0;
	bool ended = false;
	for(unsigned i = 0; i < name.length; ++i)
	{
		auto scalar = uint32_t(name.units[i]);
		if(ended)
		{
			if(scalar != 0xffff)
				return false;
			continue;
		}
		if(!scalar)
		{
			ended = true;
			continue;
		}
		if(i >= 255 || scalar == 0xffff)
			return false;
		if(scalar >= 0xd800 && scalar <= 0xdbff)
		{
			if(++i >= name.length || i >= 255 || name.units[i] < 0xdc00 || name.units[i] > 0xdfff)
				return false;
			scalar = 0x10000 + ((scalar - 0xd800) << 10) + name.units[i] - 0xdc00;
		}
		else if(scalar >= 0xdc00 && scalar <= 0xdfff)
			return false;

		if(!fatNameAppendUtf8(text, &length, G_FILENAME_MAX, scalar))
			return false;
	}
	return length && strcmp(text, ".") && strcmp(text, "..");
}

bool fatNameEncode(const char* text, uint16_t* units, size_t* count)
{
	*count = 0;
	if(!*text || !strcmp(text, ".") || !strcmp(text, ".."))
		return false;
	auto bytes = strlen(text);
	if(bytes >= G_FILENAME_MAX || text[bytes - 1] == ' ' || text[bytes - 1] == '.')
		return false;
	size_t remaining = G_FILENAME_MAX;
	while(*text)
	{
		uint32_t scalar;
		if(!fatNameReadUtf8(&text, &remaining, &scalar))
			return false;
		char encoded[5];
		size_t length = 0;
		if(!fatNameAppendUtf8(encoded, &length, sizeof(encoded), scalar))
			return false;
		if(scalar < 0x10000)
		{
			if(*count >= 255)
				return false;
			units[(*count)++] = scalar;
		}
		else
		{
			if(*count > 253)
				return false;
			scalar -= 0x10000;
			units[(*count)++] = 0xd800 | (scalar >> 10);
			units[(*count)++] = 0xdc00 | (scalar & 0x3ff);
		}
	}
	return true;
}
