#include "disk.hpp"

enum g_disk_option
{
	G_DISK_OPTION_SIZE = 1,
	G_DISK_OPTION_START = 2,
	G_DISK_OPTION_ALIGNMENT = 4,
	G_DISK_OPTION_SLOT = 8,
	G_DISK_OPTION_TYPE = 16
};

static unsigned diskHexDigit(char character)
{
	if(character >= '0' && character <= '9')
		return character - '0';
	if(character >= 'a' && character <= 'f')
		return character - 'a' + 10;
	if(character >= 'A' && character <= 'F')
		return character - 'A' + 10;
	return 16;
}

static bool diskPartitionType(const char* text, uint32_t& type)
{
	if(!strcmp(text, "fat32"))
	{
		type = 0x0c;
		return true;
	}
	if(strlen(text) != 4 || text[0] != '0' || text[1] != 'x')
		return false;
	auto high = diskHexDigit(text[2]);
	auto low = diskHexDigit(text[3]);
	if(high == 16 || low == 16)
		return false;
	auto value = high * 16 + low;
	if(!value || value == 5 || value == 15 || value == 0x85 || value == 0xee)
		return false;
	type = value;
	return true;
}

static bool diskPartitionOption(const char* key, const char* value, uint32_t sectorSize,
								g_volume_admin_request& request, unsigned& option)
{
	if(!strcmp(key, "--size"))
	{
		option = G_DISK_OPTION_SIZE;
		if(!strcmp(value, "remaining"))
		{
			request.flags |= G_DISK_PARTITION_REMAINING;
			return true;
		}
		return storageSectors(value, sectorSize, &request.sectorCount);
	}
	if(!strcmp(key, "--start"))
	{
		option = G_DISK_OPTION_START;
		request.flags |= G_DISK_PARTITION_HAS_START;
		return storageSectors(value, sectorSize, &request.firstSector);
	}
	if(!strcmp(key, "--align"))
	{
		option = G_DISK_OPTION_ALIGNMENT;
		return storageSectors(value, sectorSize, &request.alignment);
	}
	if(!strcmp(key, "--slot"))
	{
		option = G_DISK_OPTION_SLOT;
		uint64_t slot;
		if(!coreParseUnsigned(value, &slot, 4) || !slot)
			return false;
		request.slot = slot;
		return true;
	}
	if(!strcmp(key, "--type"))
	{
		option = G_DISK_OPTION_TYPE;
		return diskPartitionType(value, request.type);
	}
	return false;
}

bool diskPartitionOptions(int argc, char** argv, const g_disk_info& disk, g_volume_admin_request* request)
{
	unsigned seen = 0;
	for(int index = 4; index < argc; index += 2)
	{
		if(index + 1 == argc)
			return false;
		unsigned option = 0;
		if(!diskPartitionOption(argv[index], argv[index + 1], disk.sectorSize, *request, option) || (seen & option))
			return false;
		seen |= option;
	}
	return seen & G_DISK_OPTION_SIZE;
}
