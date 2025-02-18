/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2022, Max Schlüssel <lokoxe@gmail.com>                     *
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

#include "test.hpp"

#include <layout/flex_layout_manager.hpp>

#include "components/button.hpp"
#include "components/checkbox.hpp"
#include "components/cursor.hpp"
#include "components/scrollpane.hpp"
#include "components/text/text_field.hpp"
#include "components/window.hpp"
#include "layout/flow_layout_manager.hpp"
#include "layout/grid_layout_manager.hpp"

class open_executable_action_handler_t;
void open_executable_spawn(open_executable_action_handler_t* data);

class open_executable_action_handler_t : public internal_action_handler_t
{
public:
	std::string exe;
	std::string args;

	open_executable_action_handler_t(std::string exe, std::string args) :
		exe(exe), args(args)
	{
	}

	void handle(action_component_t* source)
	{
		g_create_task_d((void*) &open_executable_spawn, this);
	}
};

void open_executable_spawn(open_executable_action_handler_t* data)
{
	g_spawn(data->exe.c_str(), data->args.c_str(), "/", G_SECURITY_LEVEL_APPLICATION);
}

static int nextButtonPos = 70;

void addExecutableButton(window_t* window, std::string name, std::string exe, std::string args)
{

	button_t* openCalculatorButton = new button_t();
	openCalculatorButton->setBounds(g_rectangle(10, nextButtonPos, 270, 30));
	openCalculatorButton->getLabel().setTitle(name);
	openCalculatorButton->setInternalActionHandler(new open_executable_action_handler_t(exe, args));
	window->addChild(openCalculatorButton);
	nextButtonPos += 35;
}

class create_test_window_handler_t : public internal_action_handler_t
{
public:
	void handle(action_component_t* source);
};

void createTestWindow()
{
	window_t* window = new window_t;
	window->setTitle("Components");
	window->setBounds(g_rectangle(530, 30, 320, 530));
	window->setLayoutManager(new grid_layout_manager_t(1, 1));

	scrollpane_t* scroller = new scrollpane_t;
	scroller->setBounds(g_rectangle(0, 0, 300, 200));
	scroller->setFixedWidth(true);

	panel_t* content = new panel_t();
	auto contentGrid = new grid_layout_manager_t(1);
	contentGrid->setRowSpace(20);
	contentGrid->setPadding(g_insets(10, 10, 10, 10));
	content->setLayoutManager(contentGrid);
	scroller->setContent(content);
	window->addChild(scroller);

	{
		panel_t* panel = new panel_t();
		panel->setLayoutManager(new grid_layout_manager_t(1, 0, 10, 10));

		label_t* info = new label_t();
		info->setTitle("Buttons:");
		panel->addChild(info);

		button_t* button1 = new button_t();
		button1->setMinimumSize(g_dimension(0, 80));
		button1->setTitle("Button, enabled");
		panel->addChild(button1);

		button_t* button2 = new button_t();
		button2->setMinimumSize(g_dimension(0, 80));
		button2->setTitle("Button, disabled");
		button2->setEnabled(false);
		panel->addChild(button2);

		button_t* button3 = new button_t();
		button3->setTitle("Button with height from text");
		panel->addChild(button3);

		content->addChild(panel);
	}

	{
		panel_t* panel = new panel_t();
		panel->setLayoutManager(new grid_layout_manager_t(1, 0, 10, 10));

		label_t* info = new label_t();
		info->setTitle("Text fields:");
		panel->addChild(info);

		text_field_t* text = new text_field_t();
		text->setPreferredSize(g_dimension(0, 30));
		panel->addChild(text);

		text_field_t* pass = new text_field_t();
		pass->setSecure(true);
		pass->setPreferredSize(g_dimension(0, 30));
		panel->addChild(pass);

		content->addChild(panel);
	}

	{
		panel_t* panel = new panel_t();
		panel->setLayoutManager(new grid_layout_manager_t(1, 0, 10, 10));

		checkbox_t* check = new checkbox_t();
		check->getLabel().setTitle("Check me");
		panel->addChild(check);

		content->addChild(panel);
	}

	windowserver_t::instance()->screen->addChild(window);
	window->setVisible(true);
}

void createTestWindow2()
{
	window_t* window = new window_t;
	window->setTitle("Grid layout");
	window->setBounds(g_rectangle(700, 10, 300, 300));

	auto grid = new grid_layout_manager_t(3, 3, 10, 10);
	grid->setPadding(g_insets(10, 10, 10, 10));
	window->setLayoutManager(grid);

	for(int i = 0; i < 9; i++)
	{
		button_t* button1 = new button_t();
		std::stringstream s;
		s << "Button " << i;
		button1->setTitle(s.str().c_str());
		window->addChild(button1);
	}

	windowserver_t::instance()->screen->addChild(window);
	window->setVisible(true);
}

void createTestWindow3()
{
	window_t* window = new window_t;
	window->setTitle("Flex layout");
	window->setBounds(g_rectangle(700, 10, 300, 300));

	auto flex = new flex_layout_manager_t();
	flex->setHorizontal(false);
	window->setLayoutManager(flex);

	button_t* button = new button_t();
	button->setTitle("Button1");
	window->addChild(button);
	flex->setLayoutInfo(button, 0.0f, 1.0f, 100);

	button_t* button2 = new button_t();
	button2->setTitle("Button2");
	window->addChild(button2);
	flex->setLayoutInfo(button2, 1.0f, 1.0f, -1);

	windowserver_t::instance()->screen->addChild(window);
	window->setVisible(true);
}

void create_test_window_handler_t::handle(action_component_t* source)
{
	g_create_task((void*) &createTestWindow);
}

void test_t::createTestComponents()
{
	// createTestWindow();
	// createTestWindow2();
	// createTestWindow3();
}
