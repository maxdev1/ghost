#include "volume.hpp"

static int volumeToolControl(bool mount, g_volume_info info, const char* name)
{
	auto id = info.id;
	bool wasAvailable = info.state == G_VOLUME_AVAILABLE;
	auto status = mount ? volumeMount(id, name, &info) : volumeUnmount(id, &info, info.claim);
	if(status != G_VOLUME_OK)
	{
		fprintf(stderr, "volume: cannot %s volume %llu: %s\n", mount ? "mount" : "unmount",
				static_cast<unsigned long long>(id), volumeToolStatusText(status));
		if(status != G_VOLUME_TIMED_OUT && status != G_VOLUME_UNAVAILABLE && status != G_VOLUME_PROTOCOL_ERROR &&
		   info.state == G_VOLUME_MOUNTED && info.path[0])
			fprintf(stderr, "volume: volume %llu is mounted at %s\n", static_cast<unsigned long long>(id), info.path);
		if(!mount && status == G_VOLUME_IO_ERROR && info.state == G_VOLUME_AVAILABLE)
			fprintf(stderr, "volume: volume %llu is now unmounted despite the error\n",
					static_cast<unsigned long long>(id));
		return 1;
	}
	if(mount)
	{
		if(info.state != G_VOLUME_MOUNTED || !info.path[0])
		{
			fprintf(stderr, "volume: manager did not confirm the mount\n");
			return 1;
		}
		printf("Volume %llu mounted at %s.\n", static_cast<unsigned long long>(id), info.path);
	}
	else
	{
		if(info.state != G_VOLUME_AVAILABLE)
		{
			fprintf(stderr, "volume: manager did not confirm the unmount\n");
			return 1;
		}
		printf(wasAvailable ? "Volume %llu is not mounted.\n" : "Unmounted volume %llu.\n",
			   static_cast<unsigned long long>(id));
	}
	return 0;
}

static int volumeToolRunMutation(g_volume_id id, g_volume_admin_request request)
{
	g_volume_info info;
	if(!volumeToolGet(id, info))
		return 1;
	g_disk_info disk;
	auto status = diskGet(info.device, &disk);
	if(status != G_VOLUME_OK)
		return storageError("volume", status);
	request.generation = disk.generation;
	int result = storageRunJob("volume", request);
	if(!result && request.command == G_VOLUME_FORMAT)
	{
		auto started = g_millis();
		while(volumeGet(id, &info) == G_VOLUME_OK && info.handler == G_TID_NONE && g_millis() - started < 10000)
			g_sleep(10);
		if(info.handler == G_TID_NONE)
		{
			fprintf(stderr, "volume: formatted; filesystem recognition is still pending\n");
			return 1;
		}
		printf("Volume %llu formatted and unmounted.\n", (unsigned long long) id);
	}
	return result;
}

int volumeToolRun(int argc, char** argv)
{
	const char* usage = "Usage: volume list\n       volume ID get\n       volume ID mount [NAME]\n"
						"       volume ID|NAME|/mount/NAME unmount\n       volume ID format fat32 [--label LABEL]\n"
						"       volume ID wipe --zero\n       volume job ID get\n";
	if(argc == 2 && coreArgumentIsHelp(argv[1]))
	{
		printf("%sNames select paths under /mount. Format and wipe require unmounted volumes.\n", usage);
		return 0;
	}
	if(argc == 2 && !strcmp(argv[1], "list"))
		return volumeToolList();
	if(argc == 4 && !strcmp(argv[1], "job") && !strcmp(argv[3], "get"))
		return storageJobQuery("volume", argv[2]);
	if(argc == 3 && !strcmp(argv[2], "unmount"))
	{
		g_volume_info info;
		return volumeToolResolve(argv[1], info) ? volumeToolControl(false, info, nullptr) : 1;
	}
	g_volume_id id;
	if(argc < 3 || !volumeToolParseId(argv[1], id))
	{
		fprintf(stderr, "%s", usage);
		return 1;
	}
	if((argc == 3 || argc == 4) && !strcmp(argv[2], "mount"))
	{
		const char* name = argc == 4 ? argv[3] : nullptr;
		if(name && !volumeMountNameValid(name))
			return storageError("volume", G_VOLUME_INVALID);
		g_volume_info info;
		return volumeToolGet(id, info) ? volumeToolControl(true, info, name) : 1;
	}
	if(argc == 3 && !strcmp(argv[2], "get"))
	{
		g_volume_info info;
		if(!volumeToolGet(id, info))
			return 1;
		volumeToolPrintInfo(info);
		return 0;
	}
	g_volume_admin_request request;
	request.volume = id;
	if((argc == 4 || argc == 6) && !strcmp(argv[2], "format") && !strcmp(argv[3], "fat32"))
	{
		request.command = G_VOLUME_FORMAT;
		strcpy(request.filesystem, "fat32");
		if(argc == 6)
		{
			if(strcmp(argv[4], "--label") || strlen(argv[5]) >= sizeof(request.label))
				return storageError("volume", G_VOLUME_INVALID);
			strcpy(request.label, argv[5]);
		}
	}
	else if(argc == 4 && !strcmp(argv[2], "wipe") && !strcmp(argv[3], "--zero"))
		request.command = G_VOLUME_WIPE;
	else
	{
		fprintf(stderr, "%s", usage);
		return 1;
	}
	return volumeToolRunMutation(id, request);
}
