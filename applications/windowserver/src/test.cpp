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
#include "components/background.hpp"
#include "components/button.hpp"
#include "components/checkbox.hpp"
#include "components/cursor.hpp"
#include "components/plain_console_panel.hpp"
#include "components/scrollbar.hpp"
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
    open_executable_action_handler_t(std::string exe, std::string args) : exe(exe), args(args)
    {
    }
    void handle(action_component_t* source)
    {
        g_create_thread_d((void*) &open_executable_spawn, this);
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
    window_t* testWindow = new window_t;
    testWindow->setTitle("Welcome to Ghost!");
    testWindow->setBounds(g_rectangle(10, 10, 320, 430));
    // testWindow->setNumericProperty(G_UI_PROPERTY_RESIZABLE, false);

    label_t* infoLabel = new label_t();
    infoLabel->setBounds(g_rectangle(10, 10, 300, 20));
    infoLabel->setTitle("This is a demo launchpad for executing");
    infoLabel->setColor(RGB(0, 0, 0));
    testWindow->addChild(infoLabel);

    label_t* infoLabel2 = new label_t();
    infoLabel2->setBounds(g_rectangle(10, 30, 300, 20));
    infoLabel2->setTitle("the available GUI applications.");
    infoLabel2->setColor(RGB(0, 0, 0));
    testWindow->addChild(infoLabel2);

    addExecutableButton(testWindow, "Calculator", "/applications/calculator.bin", "");
    addExecutableButton(testWindow, "Terminal", "/applications/terminal2.bin", "");
    addExecutableButton(testWindow, "Drawing demo", "/applications/tetris.bin", "");

    text_field_t* testField = new text_field_t();
    testField->setBounds(g_rectangle(10, nextButtonPos, 270, 30));
    testWindow->addChild(testField);
    nextButtonPos += 35;

    button_t* createAnotherButton = new button_t();
    createAnotherButton->setBounds(g_rectangle(10, nextButtonPos, 270, 30));
    createAnotherButton->getLabel().setTitle("Create another window");
    createAnotherButton->setInternalActionHandler(new create_test_window_handler_t());
    testWindow->addChild(createAnotherButton);

    windowserver_t::instance()->screen->addChild(testWindow);
    testWindow->setVisible(true);
}

void create_test_window_handler_t::handle(action_component_t* source)
{
    g_create_thread((void*) &createTestWindow);
}

void test_t::createTestComponents()
{
    createTestWindow();

    window_t* secondWindow = new window_t;
    secondWindow->setTitle("Scroller");
    secondWindow->setBounds(g_rectangle(200, 200, 500, 500));
    secondWindow->setLayoutManager(new grid_layout_manager_t(1, 1));

    scrollpane_t* scroller = new scrollpane_t;
    scroller->setBounds(g_rectangle(0, 0, 300, 200));
    secondWindow->addChild(scroller);

    panel_t* contentPanel = new panel_t();
    contentPanel->setBounds(g_rectangle(0, 0, 400, 400));
    contentPanel->setBackground(RGB(200, 200, 200));
    contentPanel->setLayoutManager(new grid_layout_manager_t(1, 5));
    scroller->setViewPort(contentPanel);

    button_t* button1 = new button_t();
    button1->getLabel().setTitle("Button 1");
    contentPanel->addChild(button1);

    label_t* label1 = new label_t();
    label1->setTitle("This is a panel with some scrollable content. The content is layouted using a grid layout.");
    contentPanel->addChild(label1);

    label_t* label2 = new label_t();
    label2->setTitle("The height of the content panel is used to specify the scrollable area.");
    contentPanel->addChild(label2);

    button_t* button2 = new button_t();
    button2->getLabel().setTitle("Button 2");
    contentPanel->addChild(button2);

    label_t* label3 = new label_t();
    label3->setTitle("Yey! Works great :)");
    contentPanel->addChild(label3);

    windowserver_t::instance()->screen->addChild(secondWindow);
    secondWindow->setVisible(true);
}