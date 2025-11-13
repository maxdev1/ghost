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

#include <libfenster/components/button.hpp>
#include <libfenster/application.hpp>
#include <libfenster/components/window.hpp>
#include <libfenster/layout/grid_layout.hpp>
#include <libfenster/components/text_box.hpp>
#include <fstream>
#include <string>
#include <iterator>

using namespace fenster;

int main(int argc, char* argv[])
{
	if(Application::open() != ApplicationOpenStatus::Success)
	{
		klog("failed to create UI");
		return -1;
	}

	Window* window = Window::create();
	window->setBounds(Rectangle(0, 0, 600, 400));
	window->setTitle("Editor");
	window->onClose([]() { g_exit(0); });

	auto windowLayout = GridLayout::create(window);

	auto textArea = TextBox::create();
	textArea->setMultiLine(true);
	window->addChild(textArea);

	if(argc > 1)
	{
		std::ifstream file(argv[1], std::ios::binary); // open in binary mode
		if(file)
		{
			textArea->setTitle(std::string(std::istreambuf_iterator<char>(file),
										   std::istreambuf_iterator<char>()).c_str());
		}
	}

	window->setVisible(true);
	window->onClose([]()
	{
		g_exit(0);
	});

	for(;;)
		g_sleep(9999);
}
