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
#include <string>
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

	uint8_t buf[G_PIPE_DEFAULT_CAPACITY];
	std::string text;
	constexpr size_t maxLogLength = 1000;
	for(;;)
	{
		// Drain the buffered output before making synchronous UI requests.
		int r = g_read(logPipe, buf, sizeof(buf));
		if(r > 0)
		{
			text.append(reinterpret_cast<const char*>(buf), r);
			if(text.size() > maxLogLength)
				text.erase(0, text.size() - maxLogLength);

			textArea->setTitle(text);
			lastRead->setTitle(std::to_string(text.size()));
		}

		// Coalesce new output and avoid busy polling when the pipe is empty.
		g_sleep(16);
	}
}
