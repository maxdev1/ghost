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

#include <libwindow/button.hpp>
#include <libwindow/ui.hpp>
#include <libwindow/window.hpp>
#include <libwindow/interface.hpp>
#include <libwindow/textfield.hpp>
#include <libwindow/panel.hpp>
#include <libwindow/scrollpane.hpp>
#include <libwindow/label.hpp>
#include <libwindow/image.hpp>
#include <libwindow/listener/key_listener.hpp>

struct file_entry_t
{
	std::string name;
	g_fs_node_type type;
};

std::string currentBase = "/";

g_scrollpane* scroller;
g_panel* content;
g_textfield* navText;
g_button* navPrev;
g_button* navNext;
g_button* navUp;

std::vector<g_panel*> panels;
std::vector<g_panel*> selectedPanels;
g_user_mutex selectedPanelsLock = g_mutex_initialize_r(true);

std::vector<std::string> history;
int historyCurrentPosition = -1;
g_user_mutex historyLock = g_mutex_initialize_r(true);

void navigatorLoad(bool keepHistory = false);

int main()
{
	if(g_ui::open() != G_UI_OPEN_STATUS_SUCCESSFUL)
	{
		klog("failed to create UI");
		return -1;
	}

	g_window* window = g_window::create();
	window->setBounds(g_rectangle(0, 0, 600, 500));
	window->setTitle("Navigator");
	window->onClose([]() { g_exit(0); });

	window->setLayout(G_UI_LAYOUT_MANAGER_FLEX);
	window->setFlexOrientation(false);

	g_panel* navBar = g_panel::create();
	navBar->setLayout(G_UI_LAYOUT_MANAGER_FLEX);
	navBar->setLayoutPadding(g_insets(5, 5, 5, 5));
	navBar->setFlexGap(10);
	{
		navPrev = g_button::create();
		navPrev->setTitle("<");
		navBar->addChild(navPrev);
		navBar->setFlexComponentInfo(navPrev, 0, 1, 50);
		navPrev->setActionListener([]()
		{
			if(historyCurrentPosition > 0)
			{
				historyCurrentPosition--;
				currentBase = history[historyCurrentPosition];
				navigatorLoad(true);
			}
		});

		navNext = g_button::create();
		navNext->setTitle(">");
		navBar->addChild(navNext);
		navBar->setFlexComponentInfo(navNext, 0, 1, 50);
		navNext->setActionListener([]()
		{
			if(historyCurrentPosition >= 0 && historyCurrentPosition < (int) history.size() - 1)
			{
				historyCurrentPosition++;
				currentBase = history[historyCurrentPosition];
				navigatorLoad(true);
			}
		});

		navText = g_textfield::create();
		navBar->addChild(navText);
		navBar->setFlexComponentInfo(navText, 1, 1, 0);
		navText->addKeyListener([](g_key_event& e)
		{
			// TODO: g_keyboard needs a layout right now, maybe key events should already send ASCII codes:
			if(e.info.scancode == 28 && e.info.pressed)
			{
				currentBase = navText->getTitle();
				navigatorLoad();
			}
			return false;
		});

		navUp = g_button::create();
		navUp->setTitle("^");
		navBar->addChild(navUp);
		navBar->setFlexComponentInfo(navUp, 0, 1, 50);

		navUp->setActionListener([]()
		{
			size_t pos = currentBase.find_last_of("/");
			if(pos != std::string::npos)
				currentBase = currentBase.substr(0, pos);
			navigatorLoad();
		});
	}
	window->addChild(navBar);
	window->setFlexComponentInfo(navBar, 0, 1, 40);

	g_panel* centerPanel = g_panel::create();
	centerPanel->setBackground(RGB(255, 255, 255));
	centerPanel->setLayout(G_UI_LAYOUT_MANAGER_GRID);
	{
		scroller = g_scrollpane::create();
		content = g_panel::create();
		content->setLayout(G_UI_LAYOUT_MANAGER_FLOW);
		content->setLayoutPadding(g_insets(5, 5, 5, 5));

		scroller->setFixed(true, false);
		scroller->setContent(content);

		centerPanel->addChild(scroller);
	}
	window->addChild(centerPanel);
	window->setFlexComponentInfo(centerPanel, 1, 1, 0);

	window->setVisible(true);
	window->onClose([]()
	{
		g_exit(0);
	});

	navigatorLoad();

	for(;;)
		g_sleep(9999);
}

void navigatorRemoveOldPanels()
{
	std::vector<g_panel*> clonedPanels(panels);
	panels.clear();
	selectedPanels.clear();
	for(auto& panel: clonedPanels)
	{
		panel->destroy();
		// TODO delete them some time
	}
}

bool navigatorCheckCurrentPath()
{
	bool validPath = false;

	char* buffer = new char[G_PATH_MAX];
	if(g_real_path(currentBase.c_str(), buffer) == G_FS_REAL_PATH_SUCCESS)
	{
		validPath = true;
		currentBase = std::string(buffer);
	}
	delete buffer;

	return validPath;
}

void navigatorLoad(bool keepHistory)
{
	if(!navigatorCheckCurrentPath())
	{
		// TODO show error state
		navText->setTitle("?");
		return;
	}
	auto dir = g_open_directory(currentBase.c_str());
	if(!dir)
	{
		// TODO show error state
		return;
	}

	if(!keepHistory)
	{
		g_mutex_acquire(historyLock);
		if(historyCurrentPosition >= 0 && historyCurrentPosition < (int) history.size() - 1)
		{
			history.erase(history.begin() + historyCurrentPosition + 1, history.end());
		}
		history.push_back(currentBase);
		historyCurrentPosition = history.size() - 1;
		g_mutex_release(historyLock);
	}

	navigatorRemoveOldPanels();
	navText->setTitle(currentBase);
	navUp->setEnabled(currentBase != std::string("/"));
	navPrev->setEnabled(historyCurrentPosition > 0);
	navNext->setEnabled(historyCurrentPosition >= 0 && historyCurrentPosition < history.size() - 1);

	std::vector<file_entry_t> children;
	g_fs_directory_entry* entry;
	while((entry = g_read_directory(dir)) != nullptr)
	{
		std::string path = currentBase + "/" + entry->name;
		g_fs_stat_data statData;
		if(g_fs_stat(path.c_str(), &statData) != G_FS_STAT_SUCCESS)
		{
			klog("failed to stat file %s", path.c_str());
		}
		children.push_back({entry->name, statData.type});
	}
	std::sort(children.begin(), children.end(), [](const file_entry_t& a, const file_entry_t& b)
	{
		if((a.type == G_FS_NODE_TYPE_FOLDER) != (b.type == G_FS_NODE_TYPE_FOLDER))
			return a.type == G_FS_NODE_TYPE_FOLDER;
		return a.name < b.name;
	});

	content->setVisible(false);
	for(auto& child: children)
	{
		std::string path = currentBase + "/" + child.name;
		auto nodeType = child.type;

		g_panel* panel = g_panel::create();
		panel->setBackground(ARGB(1, 0, 0, 0));
		panel->setMinimumSize(g_dimension(100, 100));
		panel->addMouseListener([path,nodeType,panel](g_ui_component_mouse_event* e)
		{
			if(e->type == G_MOUSE_EVENT_PRESS)
			{
				if(e->clickCount == 2)
				{
					if(nodeType == G_FS_NODE_TYPE_FOLDER)
					{
						currentBase = path;
						navigatorLoad();
					}
					else if(nodeType == G_FS_NODE_TYPE_FILE)
					{
						std::string ending = ".bin";
						if(ending.size() > path.size())
							return false;
						if(std::equal(ending.rbegin(), ending.rend(), path.rbegin()))
						{
							g_spawn(path.c_str(), "", ".", G_SECURITY_LEVEL_APPLICATION);
						}
					}
				}
				else
				{
					g_mutex_acquire(selectedPanelsLock);
					for(auto& selectedPanel: selectedPanels)
					{
						selectedPanel->setBackground(ARGB(0, 0, 0, 0));
					}
					selectedPanels.clear();
					selectedPanels.push_back(panel);
					panel->setBackground(RGB(230, 240, 255));
					g_mutex_release(selectedPanelsLock);
				}
			}
			else if(e->type == G_MOUSE_EVENT_ENTER)
			{
				panel->setBackground(RGB(230, 240, 255));
			}
			else if(e->type == G_MOUSE_EVENT_LEAVE)
			{
				if(std::find(selectedPanels.begin(), selectedPanels.end(), panel) == selectedPanels.end())
				{
					panel->setBackground(ARGB(0, 0, 0, 0));
				}
			}
		});

		panel->setFocusable(true);

		g_image* image = g_image::create();
		image->setBounds(g_rectangle(5, 0, 90, 70));
		image->loadImage(std::string("/system/graphics/navigator/") +
		                 (nodeType == G_FS_NODE_TYPE_FOLDER ? "folder.png" : "file.png"));
		panel->addChild(image);

		g_label* label = g_label::create();
		label->setBounds(g_rectangle(5, 70, 90, 25));
		label->setAlignment(g_text_alignment::CENTER);
		label->setTitle(child.name);
		panel->addChild(label);

		panels.push_back(panel);
		content->addChild(panel);
	}
	content->setVisible(true);

	// TODO trick to make it layout again
	scroller->setContent(content);

	g_close_directory(dir);
}
