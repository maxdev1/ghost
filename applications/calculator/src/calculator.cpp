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

#include <calculator.hpp>
#include <libwindow/button.hpp>
#include <libwindow/label.hpp>
#include <libwindow/listener/action_listener.hpp>
#include <libwindow/textfield.hpp>
#include <libwindow/ui.hpp>
#include <libwindow/window.hpp>

#include <algorithm>
#include <sstream>

g_textfield* display;
g_button* but1;
g_button* but2;
g_button* but3;
g_button* but4;
g_button* but5;
g_button* but6;
g_button* but7;
g_button* but8;
g_button* but9;
g_button* but0;
g_button* butPlus;
g_button* butMinus;
g_button* butMult;
g_button* butDiv;
g_button* butEq;
g_button* butClear;

double totalValue = 0;
double currentValue = 0;
int previousCommand = COM_NONE;

/**
 *
 */
class num_press_action_listener_t : public g_action_listener
{
  public:
	int number;

	num_press_action_listener_t(int num)
	{
		this->number = num;
	}

	virtual void handle_action()
	{
		pad_button_pressed(number);
	}
};

/**
 *
 */
class command_press_action_listener_t : public g_action_listener
{
  public:
	int command;

	command_press_action_listener_t(int com)
	{
		this->command = com;
	}

	virtual void handle_action()
	{
		command_pressed(command);
	}
};

/**
 *
 */
int main(int argc, char** argv)
{

	g_ui_open_status open_stat = g_ui::open();

	if(open_stat == G_UI_OPEN_STATUS_SUCCESSFUL)
	{
		g_window* window = g_window::create();
		window->setTitle("Calculator");
		window->setResizable(false);

		display = g_textfield::create();
		display->setTitle("0");
		display->setBounds(g_rectangle(10, 10, 150, 30));
		window->addChild(display);

#define PLACE_BUTTON_T(num, text, x, y)             \
	but##num = g_button::create();                  \
	but##num->setTitle(text);                       \
	but##num->setBounds(g_rectangle(x, y, 30, 30)); \
	window->addChild(but##num);

#define PLACE_BUTTON(num, x, y) PLACE_BUTTON_T(num, #num, x, y);

		int grid1 = 10;
		int grid2 = 50;
		int grid3 = 90;
		int grid4 = 130;
		int grid5 = 170;
		int dispOff = 40;

		PLACE_BUTTON_T(Clear, "C", grid1, grid1 + dispOff);
		butClear->setActionListener(new command_press_action_listener_t(COM_CLEAR));

		PLACE_BUTTON(1, grid1, grid2 + dispOff);
		but1->setActionListener(new num_press_action_listener_t(1));
		PLACE_BUTTON(2, grid2, grid2 + dispOff);
		but2->setActionListener(new num_press_action_listener_t(2));
		PLACE_BUTTON(3, grid3, grid2 + dispOff);
		but3->setActionListener(new num_press_action_listener_t(3));

		PLACE_BUTTON(4, grid1, grid3 + dispOff);
		but4->setActionListener(new num_press_action_listener_t(4));
		PLACE_BUTTON(5, grid2, grid3 + dispOff);
		but5->setActionListener(new num_press_action_listener_t(5));
		PLACE_BUTTON(6, grid3, grid3 + dispOff);
		but6->setActionListener(new num_press_action_listener_t(6));

		PLACE_BUTTON(7, grid1, grid4 + dispOff);
		but7->setActionListener(new num_press_action_listener_t(7));
		PLACE_BUTTON(8, grid2, grid4 + dispOff);
		but8->setActionListener(new num_press_action_listener_t(8));
		PLACE_BUTTON(9, grid3, grid4 + dispOff);
		but9->setActionListener(new num_press_action_listener_t(9));

		PLACE_BUTTON(0, grid1, grid5 + dispOff);
		but0->setActionListener(new num_press_action_listener_t(0));

		PLACE_BUTTON_T(Plus, "+", grid4, grid1 + dispOff);
		butPlus->setActionListener(new command_press_action_listener_t(COM_PLUS));
		PLACE_BUTTON_T(Minus, "-", grid4, grid2 + dispOff);
		butMinus->setActionListener(new command_press_action_listener_t(COM_MINUS));
		PLACE_BUTTON_T(Mult, "*", grid4, grid3 + dispOff);
		butMult->setActionListener(new command_press_action_listener_t(COM_MULT));
		PLACE_BUTTON_T(Div, "/", grid4, grid4 + dispOff);
		butDiv->setActionListener(new command_press_action_listener_t(COM_DIV));
		PLACE_BUTTON_T(Eq, "=", grid4, grid5 + dispOff);
		butEq->setActionListener(new command_press_action_listener_t(COM_EQ));

		window->setBounds(g_rectangle(70, 70, 190, 320));
		window->setVisible(true);

		g_user_mutex lock = g_mutex_initialize();
		g_mutex_acquire(lock);
		g_mutex_acquire(lock);
	}
}

/**
 *
 */
void pad_button_pressed(int num)
{

	butPlus->setEnabled(true);
	butMinus->setEnabled(true);
	butMult->setEnabled(true);
	butDiv->setEnabled(true);

	currentValue = currentValue * 10 + num;

	std::stringstream str;
	str << currentValue;
	display->setTitle(str.str());

	if(previousCommand == COM_EQ || previousCommand == COM_CLEAR || previousCommand == COM_NONE)
	{
		totalValue = currentValue;
	}
}

/**
 *
 */
void command_pressed(int command)
{

	butPlus->setEnabled(true);
	butMinus->setEnabled(true);
	butMult->setEnabled(true);
	butDiv->setEnabled(true);

	if(previousCommand == COM_PLUS)
	{
		totalValue += currentValue;
	}
	else if(previousCommand == COM_MINUS)
	{
		totalValue -= currentValue;
	}
	else if(previousCommand == COM_MULT)
	{
		totalValue *= currentValue;
	}
	else if(previousCommand == COM_DIV)
	{
		totalValue /= currentValue;
	}
	else if(previousCommand != COM_EQ && previousCommand != COM_CLEAR)
	{
		totalValue = currentValue;
	}

	if(command == COM_PLUS)
	{
		butPlus->setEnabled(false);
	}
	else if(command == COM_MINUS)
	{
		butMinus->setEnabled(false);
	}
	else if(command == COM_MULT)
	{
		butMult->setEnabled(false);
	}
	else if(command == COM_DIV)
	{
		butDiv->setEnabled(false);
	}
	else if(command == COM_EQ)
	{
		std::stringstream ss;
		ss << totalValue;
		display->setTitle(ss.str());
	}
	else if(command == COM_CLEAR)
	{
		if(currentValue == 0)
		{
			totalValue = 0;
		}
		display->setTitle("0");
	}

	currentValue = 0;
	previousCommand = command;
}
