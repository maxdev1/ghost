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

#include <libfenster/components/button.hpp>
#include <libfenster/application.hpp>
#include <libfenster/components/window.hpp>
#include <libfenster/interface.hpp>
#include <libfenster/components/text_box.hpp>
#include <libfenster/components/panel.hpp>
#include <libfenster/components/scrollpane.hpp>
#include <libfenster/components/label.hpp>
#include <libfenster/components/image.hpp>
#include <libfenster/listener/key_listener.hpp>
#include <libfenster/layout/flex_layout.hpp>
#include <libfenster/layout/grid_layout.hpp>
#include <libfenster/layout/flow_layout.hpp>
#include <libfenster/layout/stack_layout.hpp>
#include <libfenster/component_update_batch.hpp>
#include <deque>
#include <memory>

using namespace fenster;

struct file_entry_t
{
	std::string name;
	g_fs_node_type type;
};

std::string currentBase = "/";

ScrollPane* scroller;
Panel* content;
TextBox* navText;
Button* navPrev;
Button* navNext;
Button* navUp;

// Slots grow only when a larger directory is visited; navigation reuses them.
struct file_tile_t
{
	Panel* panel;
	Image* image;
	Label* label;
	std::string path;
	std::string name;
	g_fs_node_type type = G_FS_NODE_TYPE_FILE;
	bool directory = false;
	bool active = false;
	bool hasIcon = false;
};
std::vector<std::unique_ptr<file_tile_t>> tiles;
file_tile_t* selectedTile = nullptr;
size_t visibleTileCount = 0;
bool loading = false;
g_user_mutex tilesLock = g_mutex_initialize();

std::vector<std::string> history;
int historyCurrentPosition = -1;

enum class navigation_type_t { Path, Back, Forward, Up };
struct navigation_request_t
{
	navigation_type_t type;
	std::string path;
};
std::deque<navigation_request_t> navigationRequests;
g_user_mutex navigationLock = g_mutex_initialize();
g_user_mutex navigationReady = g_mutex_initialize();

bool navigatorLoad(bool keepHistory = false);
void navigatorRun();

void navigatorRequest(navigation_type_t type, std::string path = "")
{
	g_mutex_acquire(navigationLock);
	navigationRequests.push_back({type, std::move(path)});
	g_mutex_release(navigationLock);
	g_mutex_release(navigationReady);
}

int main()
{
	g_mutex_acquire(navigationReady);
	if(Application::open() != ApplicationOpenStatus::Success)
	{
		klog("failed to create UI");
		return -1;
	}

	Window* window = Window::create();
	window->setBounds(Rectangle(0, 0, 600, 500));
	window->setTitle("Navigator");
	window->onClose([]() { g_exit(0); });

	auto windowLayout = FlexLayout::create(window);
	windowLayout->setOrientation(Orientation::Vertical);

	Panel* navBar = Panel::create();
	auto navBarLayout = FlexLayout::create(navBar);
	navBarLayout->setOrientation(Orientation::Horizontal);
	navBarLayout->setPadding(Insets(5, 5, 5, 5));
	navBarLayout->setSpace(10);
	{
		navPrev = Button::create();
		navPrev->setVariant(ButtonVariant::Ghost);
		navPrev->setTitle("<");
		navBar->addChild(navPrev);
		navBarLayout->setComponentInfo(navPrev, 0, 1, 50);
		navPrev->setActionListener([]() { navigatorRequest(navigation_type_t::Back); });

		navNext = Button::create();
		navNext->setVariant(ButtonVariant::Ghost);
		navNext->setTitle(">");
		navBar->addChild(navNext);
		navBarLayout->setComponentInfo(navNext, 0, 1, 50);
		navNext->setActionListener([]() { navigatorRequest(navigation_type_t::Forward); });

		navText = TextBox::create();
		navBar->addChild(navText);
		navBarLayout->setComponentInfo(navText, 1, 1, -1);
		navText->addKeyListener([](KeyEvent& e)
		{
			// TODO: g_keyboard needs a layout right now, maybe key events should already send ASCII codes:
			if(e.info.scancode == 28 && e.info.pressed)
			{
				navigatorRequest(navigation_type_t::Path, navText->getTitle());
			}
			return false;
		});

		navUp = Button::create();
		navUp->setTitle("^");
		navUp->setVariant(ButtonVariant::Ghost);
		navBar->addChild(navUp);
		navBarLayout->setComponentInfo(navUp, 0, 1, 50);

		navUp->setActionListener([]() { navigatorRequest(navigation_type_t::Up); });
	}
	window->addChild(navBar);
	windowLayout->setComponentInfo(navBar, 0, 1, 40);

	Panel* centerPanel = Panel::create();
	centerPanel->setBackground(_RGB(255, 255, 255));
	{
		GridLayout::create(centerPanel);
		scroller = ScrollPane::create();
		content = Panel::create();

		auto contentLayout = FlowLayout::create(content);
		contentLayout->setPadding(Insets(5, 5, 5, 5));

		scroller->setFixed(true, false);
		scroller->setContent(content);

		centerPanel->addChild(scroller);
	}
	window->addChild(centerPanel);
	windowLayout->setComponentInfo(centerPanel, 1, 1, -1);

	window->setVisible(true);
	window->onClose([]()
	{
		g_exit(0);
	});

	navigatorLoad();
	navigatorRun();
}

static bool isDirectory(g_fs_node_type type)
{
	return type == G_FS_NODE_TYPE_FOLDER || type == G_FS_NODE_TYPE_ROOT || type == G_FS_NODE_TYPE_MOUNTPOINT;
}

static void handleTileMouse(file_tile_t* tile, ComponentMouseEvent* event)
{
	std::string path;
	bool directory = false;
	bool launch = false;
	g_mutex_acquire(tilesLock);
	if(loading || !tile->active)
	{
		g_mutex_release(tilesLock);
		return;
	}
	ComponentUpdateBatch updates;
	if(event->type == MouseEventType::Press)
	{
		if(event->clickCount == 2)
		{
			path = tile->path;
			directory = tile->directory;
			launch = directory || tile->type == G_FS_NODE_TYPE_FILE;
		}
		else
		{
			if(selectedTile && selectedTile != tile)
				updates.setNumericProperty(selectedTile->panel, ComponentProperty::Background, ARGB(0, 0, 0, 0));
			selectedTile = tile;
			updates.setNumericProperty(tile->panel, ComponentProperty::Background, _RGB(230, 240, 255));
		}
	}
	else if(event->type == MouseEventType::Enter)
		updates.setNumericProperty(tile->panel, ComponentProperty::Background, _RGB(230, 240, 255));
	else if(event->type == MouseEventType::Leave && selectedTile != tile)
		updates.setNumericProperty(tile->panel, ComponentProperty::Background, ARGB(0, 0, 0, 0));
	updates.commit();
	g_mutex_release(tilesLock);

	if(launch)
	{
		if(directory)
			navigatorRequest(navigation_type_t::Path, path);
		else if(path.size() >= 4 && path.compare(path.size() - 4, 4, ".bin") == 0)
			g_spawn(path.c_str(), "", ".", G_SECURITY_LEVEL_APPLICATION);
		else
			g_spawn("/applications/editor.bin", path.c_str(), ".", G_SECURITY_LEVEL_APPLICATION);
	}
}

static bool ensureTiles(size_t count, ComponentUpdateBatch& updates)
{
	if(count <= tiles.size())
		return true;
	const auto missing = count - tiles.size();
	const auto panels = Component::createComponents<Panel, ComponentType::Panel>(missing);
	const auto images = Component::createComponents<Image, ComponentType::Image>(missing);
	const auto labels = Component::createComponents<Label, ComponentType::Label>(missing);
	if(panels.size() != missing || images.size() != missing || labels.size() != missing)
		return false;
	for(size_t i = 0; i < missing; ++i)
	{
		auto tile = std::make_unique<file_tile_t>();
		tile->panel = panels[i];
		tile->image = images[i];
		tile->label = labels[i];
		auto stableTile = tile.get();
		updates.setMinimumSize(tile->panel, Dimension(100, 100));
		updates.setNumericProperty(tile->panel, ComponentProperty::Focusable, true);
		updates.addMouseListener(tile->panel, [stableTile](ComponentMouseEvent* event) { handleTileMouse(stableTile, event); });
		updates.setBounds(tile->image, Rectangle(5, 0, 90, 70));
		updates.addChild(tile->panel, tile->image);
		updates.setBounds(tile->label, Rectangle(5, 70, 90, 25));
		updates.setNumericProperty(tile->label, ComponentProperty::Alignment, static_cast<uint32_t>(TextAlignment::CENTER));
		updates.addChild(tile->panel, tile->label);
		updates.addChild(content, tile->panel);
		tiles.push_back(std::move(tile));
	}
	return true;
}

bool navigatorLoad(bool keepHistory)
{
	char canonicalPath[G_PATH_MAX];
	if(g_real_path(currentBase.c_str(), canonicalPath) != G_FS_REAL_PATH_SUCCESS)
	{
		navText->setTitle("?");
		return false;
	}
	auto dir = g_open_directory(canonicalPath);
	if(!dir)
		return false;
	currentBase = canonicalPath;

	std::vector<file_entry_t> children;
	g_fs_directory_entry* entry;
	while((entry = g_read_directory(dir)) != nullptr)
		children.push_back({entry->name, entry->type});
	g_close_directory(dir);
	std::sort(children.begin(), children.end(), [](const file_entry_t& a, const file_entry_t& b)
	{
		if(isDirectory(a.type) != isDirectory(b.type))
			return isDirectory(a.type);
		return a.name < b.name;
	});

	if(!keepHistory)
	{
		history.erase(history.begin() + historyCurrentPosition + 1, history.end());
		history.push_back(currentBase);
		historyCurrentPosition = static_cast<int>(history.size()) - 1;
	}

	g_mutex_acquire(tilesLock);
	loading = true;
	selectedTile = nullptr;
	g_mutex_release(tilesLock);
	content->setVisible(false);
	ComponentUpdateBatch updates;
	if(!ensureTiles(children.size(), updates))
	{
		klog("failed to create folder tiles");
		g_exit(1);
	}
	updates.setStringProperty(navText, ComponentProperty::Title, currentBase);
	updates.setNumericProperty(navUp, ComponentProperty::Enabled, currentBase != "/");
	updates.setNumericProperty(navPrev, ComponentProperty::Enabled, historyCurrentPosition > 0);
	updates.setNumericProperty(navNext, ComponentProperty::Enabled, historyCurrentPosition + 1 < static_cast<int>(history.size()));
	for(size_t i = 0; i < children.size(); ++i)
	{
		auto& tile = *tiles[i];
		const auto& child = children[i];
		const auto directory = isDirectory(child.type);
		if(tile.name != child.name)
			updates.setStringProperty(tile.label, ComponentProperty::Title, child.name);
		if(!tile.hasIcon || tile.directory != directory)
			updates.setStringProperty(tile.image, ComponentProperty::ImageSource,
				std::string("/system/graphics/navigator/") + (directory ? "folder.png" : "file.png"));
		updates.setNumericProperty(tile.panel, ComponentProperty::Background, ARGB(0, 0, 0, 0));
		if(!tile.active)
			updates.setNumericProperty(tile.panel, ComponentProperty::Visible, true);
		tile.path = currentBase + (currentBase == "/" ? "" : "/") + child.name;
		tile.name = child.name;
		tile.type = child.type;
		tile.directory = directory;
		tile.hasIcon = true;
		tile.active = true;
	}
	for(size_t i = children.size(); i < visibleTileCount; ++i)
	{
		updates.setNumericProperty(tiles[i]->panel, ComponentProperty::Visible, false);
		tiles[i]->active = false;
	}
	visibleTileCount = children.size();
	if(!updates.commit())
	{
		klog("failed to update folder tiles");
		g_exit(1);
	}
	content->setVisible(true);
	scroller->setContent(content);
	g_mutex_acquire(tilesLock);
	loading = false;
	g_mutex_release(tilesLock);
	return true;
}

void navigatorRun()
{
	for(;;)
	{
		g_mutex_acquire(navigationReady);
		for(;;)
		{
			g_mutex_acquire(navigationLock);
			if(navigationRequests.empty())
			{
				g_mutex_release(navigationLock);
				break;
			}
			auto request = std::move(navigationRequests.front());
			navigationRequests.pop_front();
			g_mutex_release(navigationLock);
			const auto previousBase = currentBase;
			const auto previousPosition = historyCurrentPosition;
			switch(request.type)
			{
				case navigation_type_t::Path:
					currentBase = std::move(request.path);
					break;
				case navigation_type_t::Back:
					if(historyCurrentPosition <= 0)
						continue;
					currentBase = history[--historyCurrentPosition];
					break;
				case navigation_type_t::Forward:
					if(historyCurrentPosition + 1 >= static_cast<int>(history.size()))
						continue;
					currentBase = history[++historyCurrentPosition];
					break;
				case navigation_type_t::Up:
					currentBase = currentBase.substr(0, currentBase.find_last_of('/'));
					if(currentBase.empty())
						currentBase = "/";
					break;
			}
			if(!navigatorLoad(request.type == navigation_type_t::Back || request.type == navigation_type_t::Forward))
			{
				currentBase = previousBase;
				historyCurrentPosition = previousPosition;
			}
		}
	}
}
