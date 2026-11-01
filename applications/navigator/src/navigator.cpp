/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2025, Max Schlüssel <lokoxe@gmail.com>                     *
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

#include <algorithm>
#include <cstdio>
#include <ghost.h>
#include <libfenster/application.hpp>
#include "browser_model.hpp"
#include "browser_window.hpp"
#include "desktop/desktop.hpp"
#include "interface.hpp"

namespace
{
	/**
	 * Later command invocations forward an absolute path without opening another
	 * Fenster connection. Each fragment receives a bounded acknowledgement wait.
	 */
	int navigatorForward(int argc, char* argv[])
	{
		char cwd[G_PATH_MAX];
		if(g_get_working_directory(cwd) != G_GET_WORKING_DIRECTORY_SUCCESSFUL)
			return 1;
		auto path = navigator::resolvePath(cwd, argc > 1 ? argv[1] : "/");
		if(path.size() >= navigator::pathLimit)
		{
			fprintf(stderr, "navigator: path is too long\n");
			return 1;
		}
		auto service = g_task_get_by_name(navigator::serviceName);
		auto transaction = g_get_message_tx_id();
		for(size_t offset = 0; offset < path.size();)
		{
			navigator::OpenRequest request;
			request.total = path.size();
			request.offset = offset;
			request.length = std::min(navigator::fragmentSize, path.size() - offset);
			std::memcpy(request.path, path.data() + offset, request.length);
			if(g_send_message_tm(service, &request, sizeof(request), transaction, G_MESSAGE_SEND_MODE_NON_BLOCKING) !=
			   G_MESSAGE_SEND_STATUS_SUCCESSFUL)
				break;
			uint8_t buffer[sizeof(g_message_header) + sizeof(navigator::OpenStatus)];
			auto deadline = g_millis() + 10000;
			g_message_receive_status received;
			while((received = g_receive_message_tm(buffer, sizeof(buffer), transaction,
												   G_MESSAGE_RECEIVE_MODE_NON_BLOCKING)) ==
						  G_MESSAGE_RECEIVE_STATUS_EMPTY &&
				  g_millis() < deadline)
				g_sleep(1);
			if(received != G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
				break;
			auto header = reinterpret_cast<g_message_header*>(buffer);
			if(header->sender != service || header->length != sizeof(navigator::OpenStatus))
				break;
			navigator::OpenStatus status;
			std::memcpy(&status, G_MESSAGE_CONTENT(buffer), sizeof(status));
			offset += request.length;
			if(status == navigator::OpenStatus::Accepted && offset == path.size())
				return 0;
			if(status != navigator::OpenStatus::Continue)
				break;
		}
		fprintf(stderr, "navigator: could not contact the desktop\n");
		return 1;
	}
} // namespace

int main(int argc, char* argv[])
{
	if(!g_task_register_name(navigator::serviceName))
		return navigatorForward(argc, argv);
	if(fenster::Application::open() != fenster::ApplicationOpenStatus::Success || !navigatorStartBrowserWorker())
	{
		klog("navigator: could not initialize the desktop");
		return 1;
	}
	desktopInitialize();
	if(argc > 1)
	{
		char cwd[G_PATH_MAX];
		if(g_get_working_directory(cwd) == G_GET_WORKING_DIRECTORY_SUCCESSFUL)
			navigatorOpenWindow(navigator::resolvePath(cwd, argv[1]));
	}
	navigator::OpenRequests requests;
	for(;;)
	{
		uint8_t buffer[sizeof(g_message_header) + G_MESSAGE_MAXIMUM_MESSAGE_LENGTH];
		if(g_receive_message(buffer, sizeof(buffer)) != G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
			continue;
		auto header = reinterpret_cast<g_message_header*>(buffer);
		auto status = navigator::OpenStatus::Invalid;
		if(header->length == sizeof(navigator::OpenRequest))
		{
			navigator::OpenRequest request;
			std::memcpy(&request, G_MESSAGE_CONTENT(buffer), sizeof(request));
			std::string path;
			status = requests.interfaceReceive(header->sender, header->transaction, request, path);
			if(status == navigator::OpenStatus::Accepted)
				navigatorOpenWindow(path);
		}
		g_send_message_tm(header->sender, &status, sizeof(status), header->transaction,
						  G_MESSAGE_SEND_MODE_NON_BLOCKING);
	}
}
