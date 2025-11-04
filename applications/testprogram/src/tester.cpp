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
#include <libfenster/window.hpp>
#include <libfenster/panel.hpp>
#include <cstdio>
#include <libfenster/checkbox.hpp>
#include <libfenster/label.hpp>
#include <libfenster/layout/stack_layout.hpp>

int main(int argc, char** argv)
{
	if(fenster::Application::open() != FENSTER_APPLICATION_STATUS_SUCCESSFUL)
	{
		printf("Failed to start UI\n");
		return -1;
	}

	auto window = fenster::Window::create();
	window->onClose([]()
	{
		g_exit(0);
	});
	fenster::StackLayout::create(window);

	auto panel = fenster::Panel::create();
	panel->setBackground(_RGB(200, 200, 255));

	auto stackLayout = fenster::StackLayout::create(panel);
	stackLayout->setPadding(fenster::Insets(10, 10, 10, 10));
	stackLayout->setSpace(20);

	auto testLabel = fenster::Label::create();
	testLabel->setTitle("Choose the checkbox to test listener behaviour:");
	panel->addChild(testLabel);

	auto check = fenster::Checkbox::create();
	check->setTitle("Enable other field");
	panel->addChild(check);

	auto conditionalLabel = fenster::Label::create();
	conditionalLabel->setTitle("Conditional label");
	conditionalLabel->setVisible(false);
	panel->addChild(conditionalLabel);
	check->addCheckedListener([conditionalLabel](bool checked)
	{
		printf("Check state changed to %s\n", checked ? "checked" : "unchecked");
		conditionalLabel->setVisible(checked);
	});

	auto testLabel2 = fenster::Label::create();
	testLabel2->setTitle("Very cool.");
	panel->addChild(testLabel2);

	window->addChild(panel);

	window->setTitle("Test window");
	window->setBounds(fenster::Rectangle(70, 70, 400, 300));
	window->setVisible(true);

	for(;;)
	{
		g_sleep(999999);
	}
}
