#include "disk.hpp"

int diskToolRun(int argc, char** argv)
{
	const char* usage =
			"Usage: disk list\n       disk ID get\n       disk ID partition list\n"
			"       disk ID partition NUMBER get|delete\n       disk ID partition create --size SIZE|remaining\n"
			"           [--start OFFSET] [--align SIZE] [--slot 1..4] [--type fat32|0xNN]\n"
			"       disk ID table create mbr|gpt [--replace]\n       disk ID wipe --zero\n       disk job ID get\n";
	if(argc == 2 && coreArgumentIsHelp(argv[1]))
	{
		printf("%sSizes: sectors (s), KiB, MiB, GiB, TiB. Alignment defaults to 1MiB.\n"
			   "Creation allocates a partition; use volume ID format fat32 for its filesystem.\n"
			   "Edits support primary MBR partitions. GPT creation is unsupported.\n"
			   "Table --replace discards existing partitions and filesystems without zero-filling the disk.\n"
			   "Wipe zeroes all sectors; partition deletion only removes its table entry.\n",
			   usage);
		return 0;
	}
	if(argc == 2 && !strcmp(argv[1], "list"))
		return diskList();
	if(argc == 4 && !strcmp(argv[1], "job") && !strcmp(argv[3], "get"))
		return storageJobQuery("disk", argv[2]);
	uint64_t id;
	if(argc < 3 || !coreParseUnsigned(argv[1], &id) || !id || id > UINT16_MAX)
	{
		fprintf(stderr, "%s", usage);
		return 1;
	}
	g_disk_info disk;
	auto status = diskGet(id, &disk);
	if(status != G_VOLUME_OK)
		return storageError("disk", status);
	if(argc == 3 && !strcmp(argv[2], "get"))
	{
		diskPrint(disk);
		return 0;
	}
	if(argc == 4 && !strcmp(argv[2], "partition") && !strcmp(argv[3], "list"))
		return diskPartitionList(disk);
	g_volume_admin_request request;
	request.device = id;
	request.generation = disk.generation;
	if((argc == 5 || argc == 6) && !strcmp(argv[2], "table") && !strcmp(argv[3], "create"))
	{
		if(argc == 6 && strcmp(argv[5], "--replace"))
			return storageError("disk", G_VOLUME_INVALID);
		request.command = G_DISK_TABLE_CREATE;
		if(!strcmp(argv[4], "mbr"))
			request.layout = G_DISK_MBR;
		else if(!strcmp(argv[4], "gpt"))
			request.layout = G_DISK_GPT;
		else
			return storageError("disk", G_VOLUME_INVALID);
		if(argc == 6)
			request.flags = G_DISK_TABLE_REPLACE;
	}
	else if(argc == 4 && !strcmp(argv[2], "wipe") && !strcmp(argv[3], "--zero"))
		request.command = G_DISK_WIPE;
	else if(argc >= 6 && !strcmp(argv[2], "partition") && !strcmp(argv[3], "create"))
	{
		request.command = G_DISK_PARTITION_CREATE;
		if(!diskPartitionOptions(argc, argv, disk, &request))
			return storageError("disk", G_VOLUME_INVALID);
	}
	else if(argc == 5 && !strcmp(argv[2], "partition") && coreParseUnsigned(argv[3], &id) && id && id <= UINT32_MAX)
	{
		if(!strcmp(argv[4], "get"))
		{
			g_disk_partition_info part;
			status = diskPartitionNext(disk.device, id - 1, &part);
			if(status != G_VOLUME_OK || part.number != id)
				return storageError("disk", status == G_VOLUME_OK ? G_VOLUME_NOT_FOUND : status);
			printf("NUMBER\tVOLUME\tFIRST SECTOR\tSECTORS\tTYPE\tFLAGS\n");
			diskPrintPartition(part);
			return 0;
		}
		if(strcmp(argv[4], "delete"))
		{
			fprintf(stderr, "%s", usage);
			return 1;
		}
		request.command = G_DISK_PARTITION_DELETE;
		request.partition = id;
	}
	else
	{
		fprintf(stderr, "%s", usage);
		return 1;
	}
	return storageRunJob("disk", request);
}
