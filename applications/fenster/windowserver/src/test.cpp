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

#include <sstream>
#include <components/tree.hpp>
#include <components/tree_node.hpp>
#include <components/text/text_area.hpp>
#include <layout/flex_layout_manager.hpp>
#include <layout/stack_layout_manager.hpp>

#include "components/button.hpp"
#include "components/checkbox.hpp"
#include "components/cursor.hpp"
#include "components/scrollpane.hpp"
#include "components/text/text_field.hpp"
#include "components/window.hpp"
#include "layout/flow_layout_manager.hpp"
#include "layout/grid_layout_manager.hpp"

class open_exe_data_t;
void open_executable_spawn(open_exe_data_t* data);

class open_exe_data_t
{
public:
	std::string exe;
	std::string args;
};

void open_executable_spawn(open_exe_data_t* data)
{
	platformSpawn(data->exe.c_str(), data->args.c_str(), "/");
}

static int nextButtonPos = 70;

void addExecutableButton(window_t* window, std::string name, std::string exe, std::string args)
{

	button_t* openCalculatorButton = new button_t();
	openCalculatorButton->setBounds(g_rectangle(10, nextButtonPos, 270, 30));
	openCalculatorButton->getLabel().setTitle(name);
	openCalculatorButton->setInternalActionHandler([exe, args]()
	{
		auto data = new open_exe_data_t();
		data->exe = exe;
		data->args = args;
		platformCreateThreadWithData((void*) &open_executable_spawn, data);
	});
	window->addChild(openCalculatorButton);
	nextButtonPos += 35;
}

void createTestWindow()
{
	window_t* window = new window_t;
	window->setTitle("Components");
	window->setBounds(g_rectangle(100, 30, 320, 530));
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
		panel->setLayoutManager(new grid_layout_manager_t(2, 0, 10, 10));

		label_t* info = new label_t();
		info->setTitle("Buttons:");
		panel->addChild(info);

		button_t* button1 = new button_t();
		button1->setMinimumSize(g_dimension(0, 20));
		button1->setTitle("Button, enabled");
		panel->addChild(button1);

		button_t* button2 = new button_t();
		button2->setMinimumSize(g_dimension(0, 20));
		button2->setTitle("Button, disabled");
		button2->setEnabled(false);
		panel->addChild(button2);

		button_t* button3 = new button_t();
		button3->setTitle("Button with height from text");
		panel->addChild(button3);

		{
			button_t* b = new button_t();
			b->setTitle("Another button");
			panel->addChild(b);
		}
		{
			button_t* b = new button_t();
			b->setTitle("Another button");
			panel->addChild(b);
		}

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
		panel->setLayoutManager(new flow_layout_manager_t());

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
	auto window = new window_t;
	window->setTitle("Components");
	window->setBounds(g_rectangle(530, 30, 320, 530));
	window->setLayoutManager(new grid_layout_manager_t(1, 1));

	auto scroller = new scrollpane_t;
	scroller->setBounds(g_rectangle(0, 0, 300, 200));

	auto panel = new panel_t();
	panel->setLayoutManager(new stack_layout_manager_t());

	auto jsonInput = new text_area_t();
	jsonInput->setMinimumSize(g_dimension(500, 300));
	jsonInput->setText(R"({
	"rootNodes": [
		{
			"title": "Fruits",
			"children": [
				{"title":"Apple"},
				{"title":"Cherry"}
			]
		}
	]
})");
	panel->addChild(jsonInput);

	auto testInput = new text_field_t();
	testInput->setMinimumSize(g_dimension(100, 30));
	panel->addChild(testInput);

	auto tree = new tree_t();

	auto button = new button_t();
	button->setTitle("To tree");
	panel->addChild(button);
	button->setInternalActionHandler([tree, jsonInput]()
	{
		auto text = jsonInput->getText();
		platformLog(("setting model from JSON: " + text).c_str());
		tree->setModelFromJson(text);
	});


	std::string json = "{"
			"\"rootNodes\": ["
			"{"
			"\"id\": 1,"
			"\"title\": \"Node 1\","
			"\"children\": ["
			"{"
			"\"id\": 2,"
			"\"title\": \"Node 1.1\""
			"},"
			"{"
			"\"id\": 3,"
			"\"title\": \"Node 1.2\""
			"}"
			"]"
			"}"
			"]"
			"}";
	tree->setModelFromJson(json);

	panel->addChild(tree);

	scroller->setContent(panel);

	window->addChild(scroller);

	windowserver_t::instance()->screen->addChild(window);
	window->setVisible(true);
}

void test_t::createTestComponents()
{
	createTestWindow();
	createTestWindow2();
	createTestWindow3();
}
