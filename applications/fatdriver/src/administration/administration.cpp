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

#include "administration.hpp"
#include <cstring>
#include <ghost/system.h>
#include <ghost/tasks.h>
#include <libstorage/admin.hpp>
#include <new>

struct g_fat_format_job
{
	g_volume_admin_request request;
	g_tid client;
	g_message_transaction transaction;
	g_volume_admin_response response;
	uint64_t reported = 0;
};

static void fatAdminProgress(void* context, uint64_t done, uint64_t total)
{
	auto job = static_cast<g_fat_format_job*>(context);
	job->response.job.completed = done;
	job->response.job.total = total;
	if(done - job->reported < 64 && done != total)
		return;
	job->reported = done;
	g_send_message_tm(job->client, &job->response, sizeof(job->response), job->transaction,
					  G_MESSAGE_SEND_MODE_NON_BLOCKING);
}

static void fatAdminSendResponse(g_fat_format_job* job)
{
	while(g_send_message_tm(job->client, &job->response, sizeof(job->response), job->transaction,
							G_MESSAGE_SEND_MODE_NON_BLOCKING) == G_MESSAGE_SEND_STATUS_FULL &&
		  g_get_pid_for_tid(job->client) != G_PID_NONE)
		g_sleep(1);
}

static void fatAdminAwaitAcknowledgement(g_fat_format_job* job)
{
	uint8_t incoming[sizeof(g_message_header) + G_MESSAGE_MAXIMUM_MESSAGE_LENGTH];
	while(g_get_pid_for_tid(job->client) != G_PID_NONE)
	{
		if(g_receive_message_tm(incoming, sizeof(incoming), job->transaction, G_MESSAGE_RECEIVE_MODE_NON_BLOCKING) ==
		   G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
		{
			auto header = reinterpret_cast<g_message_header*>(incoming);
			if(header->sender == job->client && header->length == sizeof(g_volume_admin_request))
			{
				g_volume_admin_request ack;
				memcpy(&ack, G_MESSAGE_CONTENT(incoming), sizeof(ack));
				if(ack.magic == G_VOLUME_ADMIN_MAGIC && ack.version == G_VOLUME_ADMIN_VERSION &&
				   ack.command == G_VOLUME_FORMAT_ACK && ack.job == job->request.job)
					break;
			}
		}
		g_sleep(1);
	}
}

static void fatAdminFormat(void* argument)
{
	auto job = static_cast<g_fat_format_job*>(argument);
	auto& request = job->request;

	g_fat_device device;
	device.id = request.device;
	device.handler = request.type;

	auto& response = job->response;
	response.command = G_VOLUME_FORMAT_WORK;
	response.status = G_VOLUME_OK;
	response.job.id = request.job;
	response.job.phase = G_VOLUME_JOB_FORMATTING;
	fatAdminSendResponse(job);

	g_volume_status result = G_VOLUME_IO_ERROR;
	if(blockDeviceGetInfo(device.handler, device.id, &device.info) == G_BLOCK_SUCCESS &&
	   blockDeviceOpenBuffer(device.handler, &device.buffer) == G_BLOCK_SUCCESS)
	{
		uint32_t serial = uint32_t(request.job ^ g_millis());
		result = fatFormatVolume(&device, request.firstSector, request.sectorCount, serial ? serial : 1, request.label,
								 fatAdminProgress, job);
	}

	if(device.buffer.handle && !fatDeviceClose(&device))
	{
		response.job.ioSafe = 0;
		result = G_VOLUME_IO_ERROR;
	}

	response.status = result;
	response.job.result = response.status;
	response.job.state = result == G_VOLUME_OK ? G_VOLUME_JOB_SUCCEEDED : G_VOLUME_JOB_FAILED;

	fatAdminSendResponse(job);
	fatAdminAwaitAcknowledgement(job);
	delete job;
}

static bool fatAdminRequestValid(const g_volume_admin_request& request)
{
	return request.version == G_VOLUME_ADMIN_VERSION && !request.reserved && request.device &&
		   request.type != uint32_t(G_TID_NONE) && request.job &&
		   memchr(request.filesystem, 0, sizeof(request.filesystem)) && !strcmp(request.filesystem, "fat32") &&
		   memchr(request.label, 0, sizeof(request.label));
}

/**
 * Formatting is accepted only from the registered manager's process, whose
 * worker holds the disk reservation until the formatter closes its DMA window.
 * Completion acknowledgements are received by individual formatting workers.
 */
bool fatAdminReceive(const g_message_header* message, const void* content)
{
	if(message->length != sizeof(g_volume_admin_request))
		return false;

	g_volume_admin_request request;
	memcpy(&request, content, sizeof(request));
	if(request.magic != G_VOLUME_ADMIN_MAGIC)
		return false;

	auto manager = g_task_get_by_name(G_STORAGE_MANAGER_NAME);
	if(manager == G_TID_NONE || g_get_pid_for_tid(manager) == G_PID_NONE ||
	   g_get_pid_for_tid(message->sender) != g_get_pid_for_tid(manager))
		return true;

	if(request.command == G_VOLUME_FORMAT_WORK)
	{
		g_volume_admin_response response;
		response.command = G_VOLUME_FORMAT_WORK;
		response.job.id = request.job;
		if(fatAdminRequestValid(request))
		{
			auto job = new(std::nothrow) g_fat_format_job{request, message->sender, message->transaction, {}, 0};
			if(job && g_create_task_d(reinterpret_cast<void*>(fatAdminFormat), job) != G_TID_NONE)
				return true;

			delete job;
			response.status = G_VOLUME_UNAVAILABLE;
		}

		response.job.state = G_VOLUME_JOB_FAILED;
		g_send_message_tm(message->sender, &response, sizeof(response), message->transaction,
						  G_MESSAGE_SEND_MODE_NON_BLOCKING);
		return true;
	}

	g_volume_admin_response response;
	response.command = request.command;
	response.status = G_VOLUME_PROTOCOL_ERROR;
	g_send_message_tm(message->sender, &response, sizeof(response), message->transaction,
					  G_MESSAGE_SEND_MODE_NON_BLOCKING);
	return true;
}
