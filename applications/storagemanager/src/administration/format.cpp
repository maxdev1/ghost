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

#include "format.hpp"
#include <ghost/messages.h>
#include <ghost/system.h>
#include <ghost/tasks.h>
#include <string.h>

static g_tid adminFormatHandler = G_TID_NONE;

g_volume_status adminFormatRegister(g_tid sender, const g_volume_formatter_register_request* request)
{
	if(!memchr(request->filesystem, 0, sizeof(request->filesystem)) || strcmp(request->filesystem, "fat32"))
		return G_VOLUME_UNSUPPORTED;
	if(adminFormatHandler != G_TID_NONE && adminFormatHandler != sender &&
	   g_get_pid_for_tid(adminFormatHandler) != G_PID_NONE)
		return G_VOLUME_CONFLICT;
	adminFormatHandler = sender;
	return G_VOLUME_OK;
}

g_volume_status adminFormatPrepare(g_volume_admin_job* job, g_disk_layout layout, g_volume_scheme scheme)
{
	const g_volume_admin_request* request = &job->request;

	if(!memchr(request->filesystem, 0, sizeof(request->filesystem)) ||
	   !memchr(request->label, 0, sizeof(request->label)))
		return G_VOLUME_INVALID;

	if(strcmp(request->filesystem, "fat32") || adminFormatHandler == G_TID_NONE ||
	   g_get_pid_for_tid(adminFormatHandler) == G_PID_NONE)
		return G_VOLUME_UNSUPPORTED;

	if(scheme == G_VOLUME_WHOLE_DISK && layout != G_DISK_NONE)
		return G_VOLUME_CONFLICT;

	job->formatter = adminFormatHandler;
	return G_VOLUME_OK;
}

g_volume_status adminFormatRun(g_volume_admin_job* job)
{
	const g_volume_admin_request* format = &job->request;
	g_volume_format_work_request request = {};
	request.job = job->info.id;
	request.device = job->info.device;
	request.driver = job->snapshot.disk.driver;
	request.firstSector = job->firstSector;
	request.sectorCount = job->sectorCount;
	memcpy(request.filesystem, format->filesystem, sizeof(request.filesystem));
	memcpy(request.label, format->label, sizeof(request.label));

	g_message_transaction tx = g_get_message_tx_id();
	if(g_send_message_tm(job->formatter, &request, sizeof(request), tx, G_MESSAGE_SEND_MODE_NON_BLOCKING) !=
	   G_MESSAGE_SEND_STATUS_SUCCESSFUL)
		return G_VOLUME_UNAVAILABLE;

	job->changed = true;
	job->ioSafe = false;

	g_pid owner = g_get_pid_for_tid(job->formatter);
	g_tid responder = G_TID_NONE;
	uint8_t incoming[sizeof(g_message_header) + G_MESSAGE_MAXIMUM_MESSAGE_LENGTH] __attribute__((aligned(8)));
	for(;;)
	{
		if(owner == G_PID_NONE || g_get_pid_for_tid(job->formatter) != owner)
			return G_VOLUME_UNAVAILABLE;

		g_message_receive_status status = g_receive_message_tm(incoming, sizeof(incoming), tx, G_MESSAGE_RECEIVE_MODE_NON_BLOCKING);
		if(status == G_MESSAGE_RECEIVE_STATUS_EMPTY)
		{
			if(responder != G_TID_NONE && g_get_pid_for_tid(responder) == G_PID_NONE)
				return G_VOLUME_UNAVAILABLE;

			g_sleep(1);
			continue;
		}

		if(status != G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
			return G_VOLUME_UNAVAILABLE;

		const g_message_header* header = (const g_message_header*) incoming;
		if(header->sender == G_TID_NONE || g_get_pid_for_tid(header->sender) != owner)
			continue;

		responder = header->sender;
		if(header->length != sizeof(g_volume_admin_response))
			return G_VOLUME_PROTOCOL_ERROR;

		const g_volume_admin_response* response = (const g_volume_admin_response*) G_MESSAGE_CONTENT(incoming);
		if(response->magic != G_VOLUME_ADMIN_MAGIC || response->version != G_VOLUME_ADMIN_VERSION ||
		   response->command != G_VOLUME_FORMAT_WORK || response->job.id != job->info.id ||
		   response->status > G_VOLUME_PROTOCOL_ERROR || response->job.state > G_VOLUME_JOB_FAILED ||
		   response->job.ioSafe > 1)
			return G_VOLUME_PROTOCOL_ERROR;

		adminJobReportProgress(job, G_VOLUME_JOB_FORMATTING, response->job.completed, response->job.total);
		if(response->job.state == G_VOLUME_JOB_RUNNING)
			continue;

		job->ioSafe = response->job.ioSafe;
		if(responder == job->formatter)
			return response->status;

		g_volume_format_ack_request acknowledgement = {};
		acknowledgement.job = job->info.id;

		while(g_send_message_tm(responder, &acknowledgement, sizeof(acknowledgement), tx, G_MESSAGE_SEND_MODE_NON_BLOCKING) ==
					  G_MESSAGE_SEND_STATUS_FULL &&
			  g_get_pid_for_tid(responder) != G_PID_NONE)
		{
			g_sleep(1);
		}

		return response->status;
	}
}
