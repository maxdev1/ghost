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

#include "install.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <ghost/system.h>
#include <ghost/tasks.h>
#include "copy.hpp"

struct g_install_state
{
	g_device_id device;
	g_volume_info volume;
	uint64_t claim = 0;
	bool mounted = false;
	unsigned unmountAttempts = 0;
};

static g_volume_status installUnmount(g_install_state& target, const g_install_progress& report)
{
	report("Unmounting target volume", 0, 0);
	++target.unmountAttempts;
	auto status = volumeUnmount(target.volume.id, &target.volume, target.volume.claim);
	if(status == G_VOLUME_OK)
		target.mounted = false;
	return status;
}

/**
 * Detaches a possible mount before releasing the reservation on every exit.
 * Cleanup retries a failed unmount once and preserves the original failure.
 */
struct g_install_cleanup
{
	g_install_state& target;
	const g_install_progress& report;

	~g_install_cleanup()
	{
		while(target.mounted && target.unmountAttempts < 2)
			installUnmount(target, report);
		if(target.claim)
			diskRelease(target.device, target.claim);
	}
};

static const char* installVolumeStatus(g_volume_status status)
{
	switch(status)
	{
		case G_VOLUME_BUSY:
		case G_VOLUME_CONFLICT:
			return "disk is busy or its ownership/layout changed";
		case G_VOLUME_INVALID:
			return "invalid storage parameters or insufficient capacity";
		case G_VOLUME_NOT_FOUND:
			return "selected volume or filesystem driver was not found";
		case G_VOLUME_UNSUPPORTED:
			return "unsupported partition layout or filesystem";
		case G_VOLUME_TIMED_OUT:
			return "storage request timed out; inspect the disk before retrying";
		default:
			return "storage manager or filesystem operation failed";
	}
}

static void installProgress(const char* phase, uint64_t done, uint64_t total)
{
	static const char* previous = nullptr;
	static unsigned last = 101;
	auto percent = total ? unsigned(done * 100 / total) / 10 * 10 : 0;
	if(previous != phase || percent != last)
	{
		printf("%s: %u%%\n", phase, percent);
		fflush(stdout);
		klog("install: %s %u%%", phase, percent);
		previous = phase;
		last = percent;
	}
}

static int installFail(const std::string& message, std::string* failure)
{
	fprintf(stderr, "install: %s\n", message.c_str());
	if(failure)
		*failure = message;
	return 1;
}

static bool installError(const char** error, const char* message)
{
	*error = message;
	return false;
}

static bool installPrepareSource(const g_install_destination& destination, g_install_source_tree& source,
								 std::vector<uint8_t>& boot, const g_install_progress& report, const char** error)
{
	const auto& disk = destination.disk;
	bool reuse = destination.volume.id != 0;
	if(!reuse && disk.info.sectorCount < G_INSTALL_MIN_SECTORS)
		return installError(error, "disk is too small");

	source.clock = g_millis;
	source.plan.serial = uint32_t(g_millis()) ^ (uint32_t(disk.id) << 16) ^ 0x47484f53;
	if(!source.plan.serial)
		source.plan.serial = 1;
	report("Scanning the loaded filesystem", 0, 0);

	if(!installSourceCollect(&source.plan.entries, error) || !installSourcePrepare(&source, error))
		return false;
	if(!reuse && source.plan.bytes >
						 (disk.info.sectorCount - G_INSTALL_FIRST_SECTOR) * uint64_t(G_INSTALL_SECTOR_SIZE) * 95 / 100)
		return installError(error, "disk is too small for the loaded files");
	if(source.partitionPatchOffset == UINT64_MAX)
		return installError(error, "Boot assets lack a root partition selector; boot the updated live ISO.");

	auto file = fopen(G_INSTALL_BOOT_PATH, "rb");
	bool valid = file && fread(boot.data(), 1, boot.size(), file) == boot.size() && fgetc(file) == EOF && !ferror(file);
	if(file && fclose(file))
		valid = false;
	if(!valid)
		return installError(error, "Boot files are unavailable; boot the live ISO.");
	if(disk.info.sectorCount < G_INSTALL_MIN_SECTORS || disk.info.sectorCount > UINT32_MAX)
		return installError(error, "disk capacity is unsupported by the Limine BIOS installation");
	return installBootValidate(boot.data(), boot.size(), error);
}

static bool installCheckDestination(const g_install_destination& destination, std::vector<g_volume_info>& volumes,
									const char** error)
{
	const auto& disk = destination.disk;
	const auto& volume = destination.volume;
	bool reuse = volume.id != 0;
	g_volume_info info;
	g_volume_id after = 0;
	g_volume_status status;
	while((status = volumeNext(after, disk.id, &info)) == G_VOLUME_OK)
	{
		if(!strcmp(info.path, "/"))
			return installError(error, "Cannot install onto the running system disk.");
		volumes.push_back(info);
		after = info.id;
	}
	if(status != G_VOLUME_NOT_FOUND)
		return installError(error, installVolumeStatus(status));
	if(!reuse)
		return true;

	if(volumeGet(volume.id, &info) != G_VOLUME_OK || info.device != disk.id || info.firstSector != volume.firstSector ||
	   info.sectorCount != volume.sectorCount)
		return installError(error,
							"Selected volume is no longer available or its layout changed. Refresh the drive list.");

	return true;
}

static bool installConfirm(const g_install_destination& destination, bool confirmed)
{
	const auto& disk = destination.disk;
	const auto& volume = destination.volume;
	bool reuse = volume.id != 0;
	if(reuse)
		printf("Install into volume %lu on disk %u. Matching files will be replaced; the volume is not formatted.\n",
			   static_cast<unsigned long>(volume.id), unsigned(disk.id));
	else
		printf("Install onto disk %u. All partitions and files will be replaced with one FAT32 partition.\n",
			   unsigned(disk.id));

	if(confirmed)
		return true;

	printf("Type INSTALL to continue: ");
	fflush(stdout);
	char answer[32];
	if(fgets(answer, sizeof(answer), stdin) && !strcmp(answer, "INSTALL\n"))
		return true;

	printf("Installation cancelled.\n");
	return false;
}

static bool installWaitForJob(g_volume_status status, g_volume_job_info& job, const char* phase,
							  const g_install_progress& report, std::string& error)
{
	while(status == G_VOLUME_OK && job.state == G_VOLUME_JOB_RUNNING)
	{
		if(job.total)
			report(phase, job.completed, job.total);
		g_sleep(20);
		status = storageManagerJobGet(job.id, &job);
	}
	if(status == G_VOLUME_OK && job.state == G_VOLUME_JOB_SUCCEEDED)
		return true;
	char context[128];
	snprintf(context, sizeof(context), "%s (job %lu): ", phase, static_cast<unsigned long>(job.id));
	error = std::string(context) + installVolumeStatus(status == G_VOLUME_OK ? job.result : status);
	return false;
}

static bool installRunJob(g_install_state& target, g_volume_admin_request request, const char* phase,
						  const g_install_progress& report, std::string& error)
{
	report(phase, 0, 0);
	g_disk_info current;
	auto status = diskGet(target.device, &current);
	if(status != G_VOLUME_OK)
	{
		error = installVolumeStatus(status);
		return false;
	}
	request.device = target.device;
	request.generation = current.generation;
	g_volume_job_info job;
	status = storageManagerAdminStart(request, &job);
	if(!installWaitForJob(status, job, phase, report, error))
		return false;
	if(job.volume)
		target.volume.id = job.volume;
	return true;
}

static bool installFormatDisk(g_install_state& target, const g_install_progress& report, std::string& error)
{
	g_volume_admin_request request;
	request.command = G_DISK_TABLE_CREATE;
	request.flags = G_DISK_TABLE_REPLACE;
	if(!installRunJob(target, request, "Replacing disk partition table", report, error))
		return false;
	request = {};
	request.command = G_DISK_PARTITION_CREATE;
	request.flags = G_DISK_PARTITION_REMAINING;
	if(!installRunJob(target, request, "Creating target partition", report, error))
		return false;
	request = {};
	request.command = G_VOLUME_FORMAT;
	request.volume = target.volume.id;
	strcpy(request.filesystem, "fat32");
	return installRunJob(target, request, "Formatting FAT32", report, error);
}

static bool installWaitForFilesystem(g_volume_info& volume)
{
	auto started = g_millis();
	do
	{
		if(volumeGet(volume.id, &volume) != G_VOLUME_OK)
			break;
		if(volume.handler != G_TID_NONE)
			return true;
		g_sleep(10);
	} while(g_millis() - started < 10000);
	return false;
}

static bool installWriteBootloader(const g_disk_boot_info& target, const std::vector<uint8_t>& boot,
								   const g_install_progress& report, std::string& error)
{
	const char* phase = "Installing bootloader";
	report(phase, 0, boot.size());
	for(size_t offset = G_INSTALL_SECTOR_SIZE; offset < boot.size();)
	{
		auto size = std::min(G_DISK_BOOT_CHUNK_SIZE, boot.size() - offset);
		g_volume_job_info job;
		auto status = diskBootWrite(target, offset / G_INSTALL_SECTOR_SIZE, boot.data() + offset, size, &job);
		if(!installWaitForJob(status, job, phase, report, error))
			return false;
		offset += size;
		report(phase, offset, boot.size());
	}
	return true;
}

int installRun(const g_install_destination& destination, bool confirmed, std::string* failure,
			   g_install_progress progress)
{
	g_install_progress report = [&](const char* phase, uint64_t done, uint64_t total)
	{
		installProgress(phase, done, total);
		if(progress)
			progress(phase, done, total);
	};
	const char* error = nullptr;
	std::string jobError;
	g_install_source_tree source;
	std::vector<uint8_t> boot(G_INSTALL_BOOT_SIZE);
	std::vector<g_volume_info> volumes;
	if(!installPrepareSource(destination, source, boot, report, &error) ||
	   !installCheckDestination(destination, volumes, &error))
		return installFail(error, failure);
	if(!installConfirm(destination, confirmed))
		return 1;

	bool reuse = destination.volume.id != 0;
	g_install_state target{destination.disk.id, destination.volume};
	g_install_cleanup cleanup{target, report};
	report("Unmounting target disk volumes", 0, 0);
	for(auto volume: volumes)
	{
		if(volume.state != G_VOLUME_AVAILABLE && volumeUnmount(volume.id, &volume, volume.claim) != G_VOLUME_OK)
			return installFail("Disk is busy or could not be unmounted; close its files before installing.", failure);
	}
	auto status = diskClaim(target.device, &target.claim, reuse ? target.volume.id : 0);
	if(status != G_VOLUME_OK)
		return installFail(installVolumeStatus(status), failure);
	status = diskRescan(target.device);
	if(status != G_VOLUME_OK && (reuse || (status != G_VOLUME_INVALID && status != G_VOLUME_UNSUPPORTED)))
		return installFail(installVolumeStatus(status), failure);
	if(!reuse && !installFormatDisk(target, report, jobError))
		return installFail(jobError, failure);

	report("Waiting for filesystem", 0, 0);
	if(!installWaitForFilesystem(target.volume))
		return installFail("selected volume has no recognized filesystem", failure);
	status = diskClaim(target.device, &target.claim, target.volume.id);
	if(status != G_VOLUME_OK)
		return installFail(installVolumeStatus(status), failure);
	g_disk_boot_info bootTarget;
	status = diskBootGet(target.device, target.claim, target.volume.id, boot.size() / G_INSTALL_SECTOR_SIZE,
						 source.plan.serial, &bootTarget);
	if(status != G_VOLUME_OK)
		return installFail(installVolumeStatus(status), failure);
	source.plan.serial = bootTarget.diskIdentity;
	source.plan.firstSector = bootTarget.firstSector;

	report("Mounting target volume", 0, 0);
	char name[32];
	snprintf(name, sizeof(name), "install-%u", unsigned(g_get_tid()));
	target.mounted = true;
	status = volumeMount(target.volume.id, name, &target.volume);
	if(status != G_VOLUME_OK)
		return installFail(installVolumeStatus(status), failure);
	std::string path = target.volume.path;
	g_volume_filesystem_info fs;
	report("Checking free space", 0, 0);
	status = volumeStat(
			target.volume.id, &fs, [](void* context, uint64_t done, uint64_t total)
			{ (*static_cast<g_install_progress*>(context))("Checking free space", done, total); }, &report);
	if(status != G_VOLUME_OK)
		return installFail(std::string("Checking free space: ") + installVolumeStatus(status), failure);
	if(strcmp(fs.type, "fat32") || !fs.writable)
		return installFail("selected volume must be writable FAT32", failure);
	report("Checking destination files", 0, 0);
	if(!installCopyCheck(source, path, fs.allocationUnit, fs.freeBytes, &error))
		return installFail(error, failure);

	if(!installCopyFiles(&source, path, false, report, &error, reuse))
		return installFail(error, failure);
	report("Flushing target volume", 0, 0);
	status = volumeFlush(target.volume.id, &target.volume);
	if(status != G_VOLUME_OK)
		return installFail(installVolumeStatus(status), failure);
	if(!installCopyFiles(&source, path, true, report, &error))
		return installFail(error, failure);
	status = installUnmount(target, report);
	if(status != G_VOLUME_OK)
		return installFail(installVolumeStatus(status), failure);

	if(!installWriteBootloader(bootTarget, boot, report, jobError))
		return installFail(jobError, failure);
	const char* phase = "Committing boot sector";
	report(phase, 0, 0);
	g_volume_job_info job;
	status = diskBootCommit(bootTarget, boot.data(), G_DISK_BOOT_CODE_SIZE, !reuse, &job);
	if(!installWaitForJob(status, job, phase, report, jobError))
		return installFail(jobError, failure);

	report("Refreshing volumes", 0, 0);
	status = diskRescan(target.device);
	if(status != G_VOLUME_OK)
		return installFail(installVolumeStatus(status), failure);
	status = diskRelease(target.device, target.claim);
	if(status != G_VOLUME_OK)
		return installFail(installVolumeStatus(status), failure);
	target.claim = 0;
	printf("Installation complete and verified. Shut down and boot this HDD without the ISO.\n");
	klog("install: PASS disk %u volume %lu partition %u, %lu files and directories copied and verified", target.device,
		 target.volume.id, target.volume.partition, source.plan.entries.size());
	return 0;
}
