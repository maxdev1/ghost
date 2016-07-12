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

#include "login-screen.hpp"

#include <ghostuser/ui/ui.hpp>
#include <ghostuser/ui/window.hpp>
#include <ghostuser/ui/label.hpp>
#include <ghostuser/ui/canvas.hpp>
#include <ghostuser/ui/button.hpp>
#include <ghostuser/ui/textfield.hpp>
#include <ghostuser/ui/action_listener.hpp>

#include <sstream>

static g_window* loginWindow;
static g_textfield* usernameField;
static g_textfield* passwordField;

/**
 *
 */
class test_layouting_bounds_listener_t: public g_bounds_listener {
public:
	g_button* button;
	g_window* window;

	test_layouting_bounds_listener_t(g_window* win, g_button* butt) {
		window = win;
		button = butt;
	}

	/**
	 *
	 */
	virtual void handle_bounds_changed(g_rectangle b) {
		b.x = 10;
		b.y = b.height - 80;
		b.height = 40;
		b.width = b.width - 20;

		button->setBounds(b);
	}
};

/**
 *
 */
class login_button_action_listener_t: public g_action_listener {
public:

	/**
	 *
	 */
	virtual void handle_action() {

		g_window* dialog = g_window::create();

		std::string password = passwordField->getTitle();
		if (password == "test") {
			loginWindow->setVisible(false);
			g_label* label = g_label::create();
			label->setBounds(g_rectangle(10, 10, 200, 30));
			dialog->addChild(label);
			label->setTitle("Hello " + usernameField->getTitle() + "!");
			dialog->setBounds(g_rectangle(100, 100, 220, 160));

			g_button* button = g_button::create();
			button->setTitle("Click me");
			dialog->addChild(button);

			test_layouting_bounds_listener_t* layouter = new test_layouting_bounds_listener_t(dialog, button);
			dialog->setBoundsListener(layouter);
			layouter->handle_bounds_changed(dialog->getBounds());

		} else {
			g_label* label = g_label::create();
			label->setBounds(g_rectangle(10, 10, 200, 30));
			dialog->addChild(label);
			label->setTitle("Wrong password");
			dialog->setBounds(g_rectangle(100, 100, 220, 80));
		}

		dialog->setVisible(true);
	}
};

/**
 *
 */
int main(int argc, char** argv) {

	g_ui_open_status open_stat = g_ui::open();

	if (open_stat == G_UI_OPEN_STATUS_SUCCESSFUL) {
		loginWindow = g_window::create();

		loginWindow->setTitle("Login");

		g_label* headline = g_label::create();
		headline->setBounds(g_rectangle(10, 10, 200, 30));
		headline->setTitle("Ghost OS 0.5.1");
		loginWindow->addChild(headline);

		g_label* usernameLabel = g_label::create();
		usernameLabel->setBounds(g_rectangle(10, 45, 70, 30));
		usernameLabel->setTitle("Username:");
		loginWindow->addChild(usernameLabel);

		usernameField = g_textfield::create();
		usernameField->setBounds(g_rectangle(90, 45, 120, 30));
		loginWindow->addChild(usernameField);

		g_label* passwordLabel = g_label::create();
		passwordLabel->setBounds(g_rectangle(10, 80, 70, 30));
		passwordLabel->setTitle("Password:");
		loginWindow->addChild(passwordLabel);

		passwordField = g_textfield::create();
		passwordField->setBounds(g_rectangle(90, 80, 120, 30));
		passwordField->setSecure(true);
		loginWindow->addChild(passwordField);

		g_button* loginButton = g_button::create();
		loginWindow->addChild(loginButton);
		loginButton->setBounds(g_rectangle(10, 120, 200, 30));
		loginButton->setTitle("Login");

		g_canvas* canvas = g_canvas::create();
		loginWindow->addChild(canvas);
		canvas->setBounds(g_rectangle(10, 160, 200, 30));

		loginButton->setActionListener(new login_button_action_listener_t());

		g_rectangle login_window_bounds(800 / 2 - 100, 600 / 2 - 100, 220, 250);
		loginWindow->setBounds(login_window_bounds);

		loginWindow->setResizable(false);
		loginWindow->setVisible(true);

		// canvas test
		auto bufferInfo = canvas->getBuffer();
		klog("buffer: %x, size: %ix%i", bufferInfo.buffer, bufferInfo.width, bufferInfo.height);

		int x = 0;
		double rainbowR = 0;
		double rainbowG = 0.5;
		double rainbowB = 1;

		bool dir = true;
		while (true) {

			double fact = 0.05;
			if ((rainbowR == 1) && (rainbowG < 1) && (rainbowB == 0)) {
				rainbowG += fact;
			}
			if ((rainbowG == 1) && (rainbowR > 0) && (rainbowB == 0)) {
				rainbowR -= fact;
			}
			if ((rainbowG == 1) && (rainbowB < 1) && (rainbowR == 0)) {
				rainbowB += fact;
			}
			if ((rainbowB == 1) && (rainbowG > 0) && (rainbowR == 0)) {
				rainbowG -= fact;
			}
			if ((rainbowB == 1) && (rainbowR < 1) && (rainbowG == 0)) {
				rainbowR += fact;
			}
			if ((rainbowR == 1) && (rainbowB > 0) && (rainbowG == 0)) {
				rainbowB -= fact;
			}
			if (rainbowR > 1) {
				rainbowR = 1;
			} else if (rainbowR < 0) {
				rainbowR = 0;
			}
			if (rainbowG > 1) {
				rainbowG = 1;
			} else if (rainbowG < 0) {
				rainbowG = 0;
			}
			if (rainbowB > 1) {
				rainbowB = 1;
			} else if (rainbowB < 0) {
				rainbowB = 0;
			}

			if (dir) {
				++x;
			} else {
				--x;
			}

			if (dir && x > 200 - 10) {
				dir = false;

			} else if (!dir && x < 10) {
				dir = true;
			}

			cairo_surface_t* bufferSurface = cairo_image_surface_create_for_data((uint8_t*) bufferInfo.buffer, CAIRO_FORMAT_ARGB32, bufferInfo.width,
					bufferInfo.height, cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, bufferInfo.width));
			auto cr = cairo_create(bufferSurface);

			cairo_save(cr);
			cairo_set_source_rgba(cr, 0, 0, 0, 0);
			cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
			cairo_paint(cr);
			cairo_restore(cr);

			cairo_set_source_rgb(cr, rainbowR, rainbowG, rainbowB);
			cairo_rectangle(cr, x, 0, 10, 100);
			cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
			cairo_fill(cr);

			canvas->blit(g_rectangle(0, 0, bufferInfo.width, bufferInfo.height));

			g_sleep(10);
		}

		uint8_t blocker = true;
		g_atomic_block(&blocker);
	}
}
