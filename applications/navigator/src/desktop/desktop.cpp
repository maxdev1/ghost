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

#include "desktop.hpp"
#include "../browser_window.hpp"
#include "background.hpp"
#include "taskbar.hpp"
#include "start_menu.hpp"
#include <algorithm>

#include <fstream>
#include <libproperties/properties.hpp>

#define USER_DESKTOP_DIR "/user/desktop/"

using namespace fenster;
background_t* background = nullptr;
taskbar_t* taskbar = nullptr;

std::vector<desktop::StartEntry> desktopLoadStartMenuApplications()
{
    const std::string directory = "/user/start-menu/";
    std::vector<std::string> files;
    if(auto iterator = g_open_directory(directory.c_str())) {
        while(auto entry = g_read_directory(iterator)) {
            std::string name = entry->name;
            if(name.size() > 9 && name.substr(name.size() - 9) == ".launcher") files.push_back(name);
        }
        g_close_directory(iterator);
    }
    std::sort(files.begin(), files.end());
    std::vector<desktop::StartEntry> applications;
    for(const auto& file : files) {
        std::ifstream stream(directory + file);
        properties::Properties properties(stream);
        desktop::StartEntry entry;
        if(desktop::launcherFromProperties(properties.getProperties(), entry)) applications.push_back(std::move(entry));
    }
    return applications;
}

void desktopInitialize()
{
	background = background_t::create();
	taskbar = taskbar_t::create();
	background->setDesktopListener([](WindowsEvent* event)
	{
		taskbar->handleDesktopEvent(event);
	});
	Application::registerDesktopCanvas(background);

	Dimension screenDimension;
	Application::getScreenDimension(screenDimension);
	taskbar->setBounds(Rectangle(0, screenDimension.height - 40, screenDimension.width, 40));
	background->addTaskbar(taskbar);
    start_menu_t::create(taskbar, desktopLoadStartMenuApplications(), [](const desktop::StartEntry& entry) -> std::string {
        if(entry.action != desktop::StartAction::Launch) {
            auto status = g_system_power(entry.action == desktop::StartAction::Shutdown ? G_POWER_SHUTDOWN : G_POWER_REBOOT);
            return g_power_status_string(status);
        }
        if(entry.application == "/applications/navigator.bin") {
            navigatorOpenWindow(entry.arguments.empty() ? "/" : entry.arguments);
            return {};
        }
        klog(("Launching: " + entry.application + (entry.arguments.empty() ? "" : " " + entry.arguments)).c_str());
        return g_spawn_poim(entry.application.c_str(), entry.arguments.c_str(), "/applications", G_SECURITY_LEVEL_APPLICATION,
            nullptr, nullptr, nullptr, G_LAUNCH_MODE_UI) == G_SPAWN_STATUS_SUCCESSFUL
            ? std::string{} : "Could not open " + entry.name;
    });

	desktopLoadItems();

}

void desktopLoadItems()
{
	auto dir = g_open_directory(USER_DESKTOP_DIR);
	if(!dir) return;

	std::vector<std::string> files;
	g_fs_directory_entry* entry;
	while((entry = g_read_directory(dir)) != nullptr)
	{
		files.push_back(entry->name);
	}
	g_close_directory(dir);
	std::sort(files.begin(), files.end());

	int next = 0;
	for(const auto& file : files)
	{
		std::string path = std::string(USER_DESKTOP_DIR) + file;

		std::ifstream cfg(path);
		properties::Properties parser(cfg);
		auto properties = parser.getProperties();

		auto item = item_t::create(properties["name"], properties["icon"], properties["application"], properties["arguments"]);
		background->addItem(item);
		item->setBounds(Rectangle(0, next++ * 100, 100, 100));
	}
	background->organize();
}
