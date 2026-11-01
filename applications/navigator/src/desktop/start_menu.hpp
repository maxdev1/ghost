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

#ifndef DESKTOP_START_MENU_HPP
#define DESKTOP_START_MENU_HPP
#include <functional>
#include <map>
#include "start_menu_model.hpp"
#include "taskbar.hpp"

class start_menu_t : public Canvas
{
	taskbar_t* dock = nullptr;
	desktop::StartMenuModel model;
	Font* font = nullptr;
	LayoutedText* textLayout = nullptr;
	std::map<std::string, cairo_surface_t*> icons;
	std::function<std::string(const desktop::StartEntry&)> launch;
	std::string error;
	bool open = false;
	void activate(int index);
	void onMouse(ComponentMouseEvent* event);
	void onKey(KeyEvent& event);

protected:
	void init(taskbar_t* dock, std::vector<desktop::StartEntry> applications,
			  std::function<std::string(const desktop::StartEntry&)> launch);

public:
	explicit start_menu_t(ComponentId id) : Component(id), Canvas(id), FocusableComponent(id) {}
	~start_menu_t() override;
	static start_menu_t* create(taskbar_t* dock, std::vector<desktop::StartEntry> applications,
								std::function<std::string(const desktop::StartEntry&)> launch);
	void show(bool visible);
	void paint();
};
#endif
