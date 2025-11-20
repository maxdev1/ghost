/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2015, Max Schlüssel <lokoxe@gmail.com>                     *
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

#include "tester.hpp"

#include <libfenster/application.hpp>
#include <libfenster/components/window.hpp>
#include <libfenster/components/panel.hpp>
#include <cstdio>
#include <libfenster/components/checkbox.hpp>
#include <libfenster/components/label.hpp>
#include <libfenster/components/text_box.hpp>
#include <libfenster/layout/grid_layout.hpp>
#include <libfenster/layout/flex_layout.hpp>

int main(int argc, char** argv)
{
	if(fenster::Application::open() != fenster::ApplicationOpenStatus::Success)
	{
		printf("Failed to start UI\n");
		return -1;
	}

	auto window = fenster::Window::create();
	window->onClose([]()
	{
		g_exit(0);
	});
	auto flex = fenster::FlexLayout::create(window);

	auto textArea = fenster::TextBox::create();
	textArea->setMultiLine(true);
	textArea->setTitle("...");
	window->addChild(textArea);
	flex->setComponentInfo(textArea, 1, 1, -1);

	auto lastRead = fenster::Label::create();
	lastRead->setTitle("Last read: -");
	window->addChild(lastRead);
	flex->setComponentInfo(lastRead, 0,0, 50);

	window->setTitle("Kernel log");
	window->setBounds(fenster::Rectangle(50, 50, 600, 600));
	window->setVisible(true);

	g_fd logPipe = g_open_log_pipe();
	if(logPipe == G_FD_NONE)
	{
		textArea->setTitle("Failed to open kernel log pipe");
		g_sleep(999999999);
	}

	uint8_t buf[1] ={0};
	std::stringstream s;
	for(;;)
	{
		int r = g_read(logPipe, buf, 1);
		if(r == 0)
		{
			// Pipe is not blocking in both directions right now...
			g_sleep(100);
			continue;
		}
		s << (char) buf[0];

		textArea->setTitle(s.str());

		if(s.str().size() > 1000)
			s.str("");

		std::stringstream out;
		out << s.str().size();
		lastRead->setTitle(out.str());
	}
}
