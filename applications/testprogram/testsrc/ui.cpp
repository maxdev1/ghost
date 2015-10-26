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

#include <iostream>
#include <string>

#include <ghost.h>
#include <string>
#include <iostream>
#include <ghostuser/io/keyboard.hpp>
#include <ghostuser/utils/logger.hpp>
#include <ghostuser/ui/ui.hpp>
#include <ghostuser/ui/window.hpp>
#include <ghostuser/ui/label.hpp>
#include <ghostuser/ui/button.hpp>
#include <ghostuser/ui/textfield.hpp>
#include <ghostuser/ui/action_listener.hpp>

#include <stdio.h>
#include <errno.h>

#include <signal.h>

class test_action_listener: public g_action_listener {
public:
	g_window* window_to_hide;

	test_action_listener(g_window* window_to_hide) :
			window_to_hide(window_to_hide) {
	}

	virtual ~test_action_listener() {
	}

	virtual void handle_action(g_action_event e) {
		//	window_to_hide->setVisible(false);
		g_window* logging_in_window = g_window::create();

		g_label* info_label = g_label::create();
		info_label->setBounds(g_rectangle(10, 10, 200, 30));
		logging_in_window->addChild(info_label);

		logging_in_window->setBounds(g_rectangle(100, 100, 220, 160));
		logging_in_window->setVisible(true);
		g_logger::log("created window");
	}
};

/**
 *
 */
int main(int argc, char* argv[]) {

	std::stringstream pause;
	bool hasPause = false;
	if(argc == 2) {
		pause << argv[1];
		hasPause = true;
	}

	if(hasPause) {
		int time;
		pause >> time;
		std::cout << "pausing for " << time << "ms before attempting to open UI" << std::endl;
		if(time >= 0 && time < 30000) {
			g_sleep(time);
		}
	} else {
		std::cout << "no pause requested (" << argc << " args supplied)" << std::endl;
	}

	g_ui_open_status open_stat = g_ui::open();

	if (open_stat == G_UI_OPEN_STATUS_SUCCESSFUL) {
		g_window* login_window = g_window::create();

		g_label* welcome_label = g_label::create();
		welcome_label->setBounds(g_rectangle(10, 10, 200, 30));
		welcome_label->setTitle("Welcome to Ghost!");
		login_window->addChild(welcome_label);

		g_label* username_label = g_label::create();
		username_label->setBounds(g_rectangle(10, 45, 70, 30));
		username_label->setTitle("Username:");
		login_window->addChild(username_label);

		g_textfield* username_textfield = g_textfield::create();
		username_textfield->setBounds(g_rectangle(90, 45, 120, 30));
		login_window->addChild(username_textfield);

		g_label* password_label = g_label::create();
		password_label->setBounds(g_rectangle(10, 80, 70, 30));
		password_label->setTitle("Password:");
		login_window->addChild(password_label);

		g_textfield* password_textfield = g_textfield::create();
		password_textfield->setBounds(g_rectangle(90, 80, 120, 30));
		login_window->addChild(password_textfield);

		g_button* login_button = g_button::create();
		login_window->addChild(login_button);
		login_button->setBounds(g_rectangle(10, 120, 200, 30));
		login_button->setTitle("Login");

		login_button->setActionListener(new test_action_listener(login_window));

		g_rectangle login_window_bounds(200, 200, 220, 160);
		login_window->setBounds(login_window_bounds);
		login_window->setVisible(true);

		// TODO
		uint8_t blocker = true;
		g_atomic_block(&blocker);
	} else {

		std::cout << "Hello! Please enter your name:" << std::endl;
		while (true) {
			std::cout << ">";
			std::flush(std::cout);
			std::string name;
			std::getline(std::cin, name);

			if (name == "exit") {
				g_raise_signal(g_get_tid(), SIGINT);

			} else if (name == "fault") {
				*((uint32_t*) 0xDEADBEEF) = 0;

			}

			std::cout << "Hi, " << name << "!" << std::endl;
		}
	}
}

