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

#include "browser_model.hpp"
#include <libfenster/application.hpp>
#include <libfenster/component_update_batch.hpp>
#include <libfenster/components/button.hpp>
#include <libfenster/components/image.hpp>
#include <libfenster/components/label.hpp>
#include <libfenster/components/panel.hpp>
#include <libfenster/components/scrollpane.hpp>
#include <libfenster/components/text_box.hpp>
#include <libfenster/components/window.hpp>
#include <libfenster/layout/fill_layout.hpp>
#include <libfenster/layout/flex_layout.hpp>
#include <libfenster/layout/flow_layout.hpp>
#include <libfenster/layout/stack_layout.hpp>
#include <deque>
#include <memory>

using namespace fenster;
using navigator::Entry;

namespace
{
const auto background = _RGB(255, 255, 255);
const auto selectedColor = _RGB(233, 237, 255);
const auto hoverColor = _RGB(244, 246, 251);
const auto mutedColor = _RGB(102, 115, 135);
constexpr int fileMargin = 12, gridTop = 8, gridStepX = 132, gridStepY = 124, gridHeight = 116;
constexpr int listTop = 4, listStep = 42, listHeight = 40;
constexpr int menuTop = 30, menuInset = 6, menuStep = 32;

enum class Action
{
	Path, Back, Forward, Up, Refresh, Filter, ClearFilter, Icons, List, SortName, SortType,
	Reverse, Hidden, Favorites, Open, Edit, Terminal, NewWindow, OpenFolderWindow, Properties, Close, Location, Find,
	Select, Hover, Unhover, Deselect, MoveSelection, Menu, MenuHover, Dismiss, Resize
};
struct Request
{
	Action action;
	std::string text;
	int value = 0;
	Rectangle bounds;
};
std::deque<Request> requests;
g_user_mutex requestLock = g_mutex_initialize();
g_user_mutex requestReady = g_mutex_initialize();
g_user_mutex tilesLock = g_mutex_initialize();

Window* window;
Panel *root, *menuBar, *sidebar, *fileBackground, *content, *scrim;
Window* properties;
Layout* contentLayout = nullptr;
bool contentList = false;
ScrollPane* scroller;
TextBox *location, *filterBox;
Label *status, *emptyState, *loadingState, *listName, *listKind;
Panel *toolbar, *listHeader, *sidebarRule;
Button *refreshButton, *clearButton;
Button *backButton, *forwardButton, *upButton, *iconButton, *listButton;
std::string currentPath = "/", query, selectedPath;
navigator::History history;
std::vector<Entry> entries, visible;
bool propertyOpen = false;
bool listView = false, showHidden = false, showFavorites = false, sortType = false, descending = false;
int rootWidth = 840, rootHeight = 540, contentWidth = 640, columns = 1, openMenu = -1, menuSelection = -1;

struct Tile
{
	Panel *panel, *imageHost;
	FlexLayout* layout = nullptr;
	FillLayout* imageLayout = nullptr;
	int mode = -1;
	Image* image;
	Label *label, *type;
	std::string path, title, name, kind;
	Icon icon = Icon::None;
	bool selected = false;
	bool active = false;
};
std::vector<std::unique_ptr<Tile>> tiles;
struct MenuItem
{
	Panel* row;
	Label* title;
	Image* icon;
	Icon defaultIcon = Icon::None;
	Action action;
	std::string text, target;
	bool enabled = true;
};
struct Menu
{
	Button* button;
	Panel* panel;
	std::vector<std::unique_ptr<MenuItem>> items;
	int x;
};
std::vector<Menu> menus;
std::vector<std::pair<Button*, std::string>> places;
std::vector<Label*> propertyLabels;

void request(Action action, std::string text = "", int value = 0, Rectangle bounds = {})
{
	g_mutex_acquire(requestLock);
	// Keep typing and resize bursts from repainting the same directory repeatedly.
	if(!requests.empty() && requests.back().action == action &&
		(action == Action::Filter || (action == Action::Resize && requests.back().value == value)))
		requests.back() = {action, std::move(text), value, bounds};
	else
		requests.push_back({action, std::move(text), value, bounds});
	g_mutex_release(requestLock);
	g_mutex_release(requestReady);
}

void focus(Component* component)
{
	CommandFocusRequest message;
	message.header.id = ProtocolCommandId::Focus;
	message.id = component->getId();
	auto tx = platformCreateMessageTransaction();
	if(!sendUiMessage(DelegateTaskId, &message, sizeof(message), tx))
	{ klog("navigator: failed to focus component"); g_exit(1); }
	uint8_t response[SYS_MESSAGE_HEADER_SIZE + sizeof(CommandFocusResponse)];
	platformReceiveMessage(response, sizeof(response), tx);
}

void handleKey(KeyEvent& event, bool textInput = false)
{
	const auto& key = event.info;
	if(!key.pressed) return;
	auto scan = key.scancode & 0xff;
	if(scan == 1) request(Action::Dismiss);
	else if(key.ctrl && scan == 38) request(Action::Location); // Ctrl+L
	else if(key.ctrl && scan == 33) request(Action::Find); // Ctrl+F
	else if(key.ctrl && scan == 49) request(Action::NewWindow); // Ctrl+N
	else if(key.ctrl && scan == 17) request(Action::Close); // Ctrl+W
	else if(key.ctrl && scan == 35) request(Action::Hidden); // Ctrl+H
	else if(key.ctrl && scan == 19) request(Action::Refresh); // Ctrl+R
	else if(key.alt && scan == 75) request(Action::Back);
	else if(key.alt && scan == 77) request(Action::Forward);
	else if(key.alt && scan == 72) request(Action::Up);
	else if(scan == 63) request(Action::Refresh); // F5
	else if(!textInput && scan == 14) request(Action::Up);
	else if(!textInput && scan == 28) request(Action::Open);
	else if(!textInput && (scan == 75 || scan == 77 || scan == 72 || scan == 80))
		request(Action::MoveSelection, "", scan);
}

Label* label(Component* parent, const std::string& title, ColorArgb color = _RGB(30, 42, 60), int size = 14)
{
	auto result = Label::create();
	result->setTitle(title);
	result->setColor(color);
	if(size != 14) result->setNumericProperty(ComponentProperty::FontSize, size);
	parent->addChild(result);
	return result;
}

Button* button(Component* parent, const std::string& title, Action action, const std::string& text = "")
{
	auto result = Button::create();
	result->setTitle(title);
	result->setVariant(ButtonVariant::Ghost);
	result->setActionListener([=]() { request(action, text); });
	result->addKeyListener([](KeyEvent& event) { handleKey(event); });
	parent->addChild(result);
	return result;
}

Button* iconControl(Component* parent, Icon icon, const std::string& hint, Action action)
{
	auto control = button(parent, "", action);
	control->setIcon(icon); control->setTooltip(hint);
	return control;
}

void surface(Panel* panel, ColorArgb color, int radius = 0, ColorArgb border = 0)
{
	panel->setBackground(color);
	panel->setNumericProperty(ComponentProperty::CornerRadius, radius);
	panel->setNumericProperty(ComponentProperty::BorderColor, border);
}

void sized(Component* component, int width, int height)
{
    component->setMinimumSize({std::max(0, width), std::max(0, height)});
    component->setMaximumSize({width, height});
}
Panel* panel(Component* parent, ColorArgb color = ARGB(0, 0, 0, 0))
{
    auto result = Panel::create(); result->setBackground(color); parent->addChild(result); return result;
}
FlexLayout* flex(Component* component, Orientation orientation, Insets padding = {}, int space = 0)
{
    auto result = FlexLayout::create(component); result->setOrientation(orientation); result->setPadding(padding); result->setSpace(space); return result;
}
void part(FlexLayout* layout, Component* child, int basis, float grow = 0, float shrink = 0)
{
    layout->setComponentInfo(child, grow, shrink, basis);
}
void gap(Component* parent, FlexLayout* layout, int size, float grow = 0)
{
    part(layout, panel(parent), size, grow);
}
void configureTile(Tile& tile)
{
    if(tile.mode == int(listView)) return;
    tile.mode = listView;
    if(!tile.layout) { tile.layout = FlexLayout::create(tile.panel); tile.imageLayout = FillLayout::create(tile.imageHost); }
    sized(tile.panel, listView ? -1 : 128, listView ? listHeight : gridHeight);
    tile.layout->setOrientation(listView ? Orientation::Horizontal : Orientation::Vertical);
    tile.layout->setPadding(listView ? Insets(0, 12, 0, 12) : Insets(4, 6, 6, 6));
    tile.layout->setSpace(listView ? 10 : 2);
    part(tile.layout, tile.imageHost, listView ? 24 : 58);
    part(tile.layout, tile.label, listView ? 0 : 25, listView ? 1 : 0, listView ? 1 : 0);
    part(tile.layout, tile.type, listView ? 136 : 18);
    tile.imageLayout->setPadding(listView ? Insets(8, 0, 8, 0) : Insets(6, 32, 0, 32));
}
void configureFiles()
{
    if(contentLayout && contentList == listView) return;
    delete contentLayout;
    contentList = listView;
    if(listView) {
        auto layout = StackLayout::create(content); layout->setOrientation(Orientation::Vertical);
        layout->setPadding({listTop, fileMargin, 12, fileMargin}); layout->setSpace(listStep - listHeight); contentLayout = layout;
    } else {
        auto layout = FlowLayout::create(content); layout->setPadding({gridTop, fileMargin, 8, fileMargin}); layout->setSpace(4);
        content->setNumericProperty(ComponentProperty::LayoutSpaceVertical, gridStepY - gridHeight); contentLayout = layout;
    }
}

void dismiss()
{
	if(openMenu < 0 && !propertyOpen) return;
	for(auto& menu : menus)
	{
		menu.panel->setVisible(false);
		menu.button->setSelected(false);
	}
	properties->setVisible(false);
	propertyOpen = false;
	scrim->setVisible(false);
	scrim->setBackground(ARGB(0, 0, 0, 0));
	openMenu = -1;
	menuSelection = -1;
}

const Entry* selection()
{
	for(const auto& entry : visible)
		if(navigator::resolvePath(currentPath, entry.name) == selectedPath) return &entry;
	return nullptr;
}

void updateStatus()
{
	auto entry = selection();
	std::string text = std::to_string(visible.size()) + (visible.size() == 1 ? " item" : " items");
	if(!query.empty()) text += " matching \"" + query + "\"";
	if(entry) text += "  |  " + entry->name + "  -  " + navigator::typeName(*entry);
	status->setColor(mutedColor);
	status->setTitle(text);
}

void select(const std::string& path)
{
	if(selectedPath == path) return;
	selectedPath = path;
	ComponentUpdateBatch batch;
	ComponentUpdateBatch::Scope updates(batch);
	for(auto& tile : tiles)
		if(tile->active)
		{
			auto selected = tile->path == path;
			if(tile->selected != selected) {
				tile->panel->setNumericProperty(ComponentProperty::Selected, selected);
				tile->selected = selected;
			}
		}
	updateStatus();
	if(!batch.commit()) { klog("navigator: failed to update selection"); g_exit(1); }
}

bool ensureTiles(size_t count, ComponentUpdateBatch& batch)
{
	if(count <= tiles.size()) return true;
	auto missing = count - tiles.size();
	auto panels = Component::createComponents<Panel, ComponentType::Panel>(missing * 2);
	auto images = Component::createComponents<Image, ComponentType::Image>(missing);
	auto labels = Component::createComponents<Label, ComponentType::Label>(missing * 2);
	if(panels.size() != missing * 2 || images.size() != missing || labels.size() != missing * 2) return false;
	for(size_t i = 0; i < missing; ++i)
	{
		auto tile = std::make_unique<Tile>();
		tile->panel = panels[i * 2]; tile->imageHost = panels[i * 2 + 1]; tile->image = images[i];
		tile->label = labels[i * 2]; tile->type = labels[i * 2 + 1];
		auto stable = tile.get();
		batch.setNumericProperty(tile->panel, ComponentProperty::Focusable, true);
		batch.addMouseListener(tile->panel, [stable](ComponentMouseEvent* event)
		{
			g_mutex_acquire(tilesLock);
			auto path = stable->active ? stable->path : "";
			g_mutex_release(tilesLock);
			if(path.empty()) return;
			if(event->type == MouseEventType::Press && (event->buttons & MouseButton::Button1))
			{
				request(Action::Select, path);
				if(event->ctrl) request(Action::OpenFolderWindow, path);
				else if(event->clickCount == 2) request(Action::Open, path);
			}
		});
		tile->panel->addKeyListener([](KeyEvent& event) { handleKey(event); });
		batch.addChild(tile->panel, tile->imageHost);
		batch.addChild(tile->imageHost, tile->image);
		batch.setNumericProperty(tile->panel, ComponentProperty::CornerRadius, 10);
		batch.setNumericProperty(tile->panel, ComponentProperty::HoverBackground, hoverColor);
		batch.setNumericProperty(tile->panel, ComponentProperty::SelectedBackground, selectedColor);
		batch.setNumericProperty(tile->label, ComponentProperty::Color, _RGB(46, 56, 75));
		batch.setNumericProperty(tile->type, ComponentProperty::FontSize, 12);
		batch.addChild(tile->panel, tile->label);
		batch.setNumericProperty(tile->type, ComponentProperty::Color, mutedColor);
		batch.addChild(tile->panel, tile->type);
		batch.addChild(content, tile->panel);
		tiles.push_back(std::move(tile));
	}
	return true;
}

void renderEntries(bool openingFolder = false)
{
	ComponentUpdateBatch batch;
	ComponentUpdateBatch::Scope updates(batch);
	visible = navigator::visibleEntries(entries, query, showHidden, sortType, descending);
	if(!selection()) selectedPath.clear();
	configureFiles();
	columns = listView ? 1 : std::max(1, (contentWidth - 2 * fileMargin + 4) / gridStepX);
	content->setVisible(false);
	g_mutex_acquire(tilesLock);
	if(!ensureTiles(visible.size(), batch))
	{
		klog("navigator: failed to create file tiles");
		g_exit(1);
	}
	for(size_t i = 0; i < tiles.size(); ++i)
	{
		auto& tile = *tiles[i];
		if(i >= visible.size())
		{
			if(tile.active) batch.setNumericProperty(tile.panel, ComponentProperty::Visible, false);
			tile.active = false;
			continue;
		}
		const auto& entry = visible[i];
		tile.path = navigator::resolvePath(currentPath, entry.name);
		const auto kind = navigator::typeName(entry);
		const auto icon = entry.directory ? Icon::Folder : kind == "Application" ? Icon::Applications : Icon::Document;
		const int width = std::max(80, contentWidth - 2 * fileMargin);
		const auto title = navigator::displayName(entry.name, listView ? std::max(8, (width - 200) / 8) : 14);
		const bool selected = tile.path == selectedPath;
		if(tile.mode != int(listView)) {
			auto alignment = uint32_t(listView ? TextAlignment::LEFT : TextAlignment::CENTER);
			batch.setNumericProperty(tile.label, ComponentProperty::Alignment, alignment);
			batch.setNumericProperty(tile.type, ComponentProperty::Alignment, alignment);
		}
		if(tile.title != title) { batch.setStringProperty(tile.label, ComponentProperty::Title, title); tile.title = title; }
		if(tile.name != entry.name) { batch.setStringProperty(tile.panel, ComponentProperty::Tooltip, entry.name); tile.name = entry.name; }
		if(tile.kind != kind) { batch.setStringProperty(tile.type, ComponentProperty::Title, kind); tile.kind = kind; }
		if(tile.icon != icon) { batch.setNumericProperty(tile.image, ComponentProperty::Icon, uint32_t(icon)); tile.icon = icon; }
		if(tile.selected != selected) { batch.setNumericProperty(tile.panel, ComponentProperty::Selected, selected); tile.selected = selected; }
		if(!tile.active) batch.setNumericProperty(tile.panel, ComponentProperty::Visible, true);
		tile.active = true;
	}
	for(auto& tile : tiles) if(tile->active) configureTile(*tile);
	if(!batch.commit()) { klog("navigator: failed to update file tiles"); g_exit(1); }
	g_mutex_release(tilesLock);
	content->setVisible(true);
	emptyState->setTitle(query.empty() ? "This folder is empty" : "No matching items. Try a different filter.");
	iconButton->setSelected(!listView);
	listButton->setSelected(listView);
	listHeader->setVisible(listView);
	clearButton->setVisible(!query.empty());
	updateStatus();
	if(!batch.commit()) { klog("navigator: failed to finish folder updates"); g_exit(1); }
	if(openingFolder)
	{
		if(!scroller->prepareForDisplay())
		{ klog("navigator: failed to prepare folder contents"); g_exit(1); }
		auto width = content->getBounds().width;
		if(width > 0 && width != contentWidth)
		{
			contentWidth = width;
			renderEntries(true);
			return;
		}
	}
	scroller->setVisible(true);
	emptyState->setVisible(visible.empty());
	loadingState->setVisible(false);
	if(!batch.commit()) { klog("navigator: failed to show folder contents"); g_exit(1); }
}

bool load(const std::string& target, int historyPosition = -1)
{
	ComponentUpdateBatch batch;
	ComponentUpdateBatch::Scope updates(batch);
	scroller->setVisible(false);
	emptyState->setVisible(false);
	loadingState->setVisible(true);
	// Hide the previous folder before filesystem work or metadata updates begin.
	if(!batch.commit()) { klog("navigator: failed to hide folder contents"); g_exit(1); }
	auto fail = [&](const std::string& message)
	{
		scroller->setVisible(true);
		emptyState->setVisible(visible.empty());
		loadingState->setVisible(false);
		status->setColor(_RGB(180, 50, 45));
		status->setTitle(message);
		location->setTitle(currentPath);
		return false;
	};
	auto path = navigator::resolvePath(currentPath, target);
	char canonical[G_PATH_MAX];
	if(path.size() >= G_PATH_MAX || g_real_path(path.c_str(), canonical) != G_FS_REAL_PATH_SUCCESS)
		return fail("Cannot open folder: " + path);
	auto directory = g_open_directory(canonical);
	if(!directory)
		return fail("Cannot read folder: " + path);
	std::vector<Entry> children;
	g_fs_read_directory_status readStatus;
	while(auto entry = g_read_directory_s(directory, &readStatus))
		children.push_back({entry->name,
			entry->type == G_FS_NODE_TYPE_FOLDER || entry->type == G_FS_NODE_TYPE_ROOT || entry->type == G_FS_NODE_TYPE_MOUNTPOINT,
			entry->type == G_FS_NODE_TYPE_FILE});
	g_close_directory(directory);
	// filesystemReadDirectory currently reports ERROR for a folder with no child list.
	if(readStatus != G_FS_READ_DIRECTORY_EOD && !children.empty())
		return fail("Could not finish reading folder: " + path);
	bool changed = currentPath != canonical;
	currentPath = canonical;
	entries = std::move(children);
	if(historyPosition >= 0) history.position = historyPosition;
	else history.record(currentPath);
	if(changed)
	{
		selectedPath.clear(); query.clear(); filterBox->setTitle("");
		scroller->setNumericProperty(ComponentProperty::ScrollOffsetY, 0);
	}
	location->setTitle(currentPath);
	window->setTitle(currentPath == "/" ? "Computer" : currentPath);
	backButton->setEnabled(history.canBack());
	forwardButton->setEnabled(history.canForward());
	upButton->setEnabled(currentPath != "/");
	for(auto& place : places)
		place.first->setSelected(place.second == currentPath);
	renderEntries(true);
	if(!batch.commit()) { klog("navigator: failed to update folder metadata"); g_exit(1); }
	return true;
}

void spawn(const std::string& program, const std::string& args = "")
{
	if(g_spawn(program.c_str(), args.c_str(), currentPath.c_str(), G_SECURITY_LEVEL_APPLICATION) != G_SPAWN_STATUS_SUCCESSFUL)
	{
		status->setColor(_RGB(180, 50, 45));
		status->setTitle("Could not open " + program);
	}
}

void openSelection(bool editor)
{
	auto entry = selection();
	if(!entry) return;
	auto path = selectedPath;
	if(entry->directory) { if(!editor && load(path)) focus(content); }
	else if(entry->file)
	{
		if(!editor && navigator::typeName(*entry) == "Application") spawn(path);
		else spawn("/applications/editor.bin", navigator::argument(path));
	}
}

void showProperties()
{
	auto entry = selection();
	if(!entry) return;
	propertyLabels[0]->setTitle(entry->name);
	propertyLabels[1]->setTitle(navigator::typeName(*entry));
	propertyLabels[2]->setTitle(selectedPath);
	std::string size;
	if(entry->file)
	{
		auto fd = g_open_f(selectedPath.c_str(), G_FILE_FLAG_MODE_READ);
		if(fd != G_FD_NONE)
		{
			auto bytes = g_length(fd);
			g_close(fd);
			if(bytes >= 0) size = std::to_string(bytes) + " bytes";
		}
	}
	propertyLabels[3]->setTitle(size);
	auto parent = window->getBounds();
	properties->setBounds({parent.x + std::max(0, (parent.width - 500) / 2), parent.y + std::max(0, (parent.height - 320) / 2), 500, 320});
	properties->setVisible(true);
	propertyOpen = true;
	focus(properties);
}

void toggleMenu(int index)
{
	bool close = openMenu == index;
	dismiss();
	if(close) { focus(content); return; }
	openMenu = index;
	auto entry = selection();
	for(auto& item : menus[index].items)
	{
		item->enabled = true;
		std::string prefix;
		switch(item->action)
		{
			case Action::Open: item->enabled = entry && (entry->directory || entry->file); break;
			case Action::Edit: item->enabled = entry && entry->file; break;
			case Action::Properties: item->enabled = entry != nullptr; break;
			case Action::Back: item->enabled = history.canBack(); break;
			case Action::Forward: item->enabled = history.canForward(); break;
			case Action::Up: item->enabled = currentPath != "/"; break;
			case Action::Icons: prefix = !listView ? "* " : "  "; break;
			case Action::List: prefix = listView ? "* " : "  "; break;
			case Action::SortName: prefix = !sortType ? "* " : "  "; break;
			case Action::SortType: prefix = sortType ? "* " : "  "; break;
			case Action::Reverse: prefix = descending ? "* " : "  "; break;
			case Action::Hidden: prefix = showHidden ? "* " : "  "; break;
			case Action::Favorites: prefix = showFavorites ? "* " : "  "; break;
			default: break;
		}
		item->title->setTitle(item->text);
		item->icon->setNumericProperty(ComponentProperty::Icon, static_cast<uint32_t>(prefix == "* " ? Icon::Check : item->defaultIcon));
		item->title->setColor(item->enabled ? _RGB(30, 42, 60) : _RGB(160, 168, 180));
		item->row->setNumericProperty(ComponentProperty::Selected, false);
	}
	menus[index].button->setSelected(true);
	scrim->setVisible(true);
	menus[index].panel->setVisible(true);
	focus(menus[index].panel);
}

void layout(Rectangle bounds)
{
    rootWidth = bounds.width; rootHeight = bounds.height;
    for(auto& menu : menus) {
        menu.x = menu.button->getBounds().x;
        menu.panel->setBounds({std::min(menu.x, std::max(0, rootWidth - 272)), 0, 272, 2 * menuInset + int(menu.items.size()) * menuStep - 2});
    }
}

void addMenuItem(Menu& menu, const std::string& text, Action action, const std::string& shortcut = "", const std::string& target = "")
{
	auto item = std::make_unique<MenuItem>();
	item->row = Panel::create(); item->title = label(item->row, text);
	item->action = action; item->text = text; item->target = target;
    sized(item->row, -1, 30);
    auto row = flex(item->row, Orientation::Horizontal, {0, 10, 0, 10}, 10);
    item->row->setFocusable(true);
    item->row->setNumericProperty(ComponentProperty::CornerRadius, 6);
    item->row->setNumericProperty(ComponentProperty::HoverBackground, _RGB(241, 244, 251));
    item->row->setNumericProperty(ComponentProperty::SelectedBackground, _RGB(233, 237, 255));
    auto iconHost = panel(item->row); auto inset = FillLayout::create(iconHost); inset->setPadding({7, 0, 7, 0});
    item->icon = Image::create(); iconHost->addChild(item->icon);
    // The title was created first; re-adding it places it after the icon host.
    item->row->addChild(item->title);
    part(row, iconHost, 16); part(row, item->title, 0, 1, 1);
    if(!shortcut.empty()) {
        auto hint = label(item->row, shortcut, mutedColor, 12); hint->setAlignment(TextAlignment::RIGHT); part(row, hint, 60);
    }

	switch(action) {
		case Action::NewWindow: item->defaultIcon = Icon::NewWindow; break;
		case Action::Open: item->defaultIcon = Icon::Folder; break;
		case Action::Edit: item->defaultIcon = Icon::Document; break;
		case Action::Terminal: item->defaultIcon = Icon::Terminal; break;
		case Action::Properties: item->defaultIcon = Icon::Info; break;
		case Action::Close: item->defaultIcon = Icon::Close; break;
		case Action::Back: item->defaultIcon = Icon::Back; break;
		case Action::Forward: item->defaultIcon = Icon::Forward; break;
		case Action::Up: item->defaultIcon = Icon::Up; break;
		case Action::Refresh: item->defaultIcon = Icon::Refresh; break;
		case Action::Location: item->defaultIcon = Icon::Folder; break;
		case Action::Path: item->defaultIcon = target == "/user" ? Icon::Home : target == "/user/desktop" ? Icon::Desktop : Icon::Computer; break;
		default: break;
	}
	int rowIndex = menu.items.size();
	int menuIndex = &menu - menus.data();
	item->row->addMouseListener([rowIndex, menuIndex, action, target](ComponentMouseEvent* event)
	{
		if(event->type == MouseEventType::Press && (event->buttons & MouseButton::Button1))
		{
			// Menu state and enablement are owned by the worker as well.
			request(Action::Menu, target, -2 - static_cast<int>(action));
		}
		else if(event->type == MouseEventType::Enter) request(Action::MenuHover, std::to_string(menuIndex), rowIndex);
	});
	item->row->addKeyListener([](KeyEvent& event) { handleKey(event); });
	menu.panel->addChild(item->row);
	menu.items.push_back(std::move(item));
}

void buildUi()
{
    ComponentUpdateBatch batch;
    ComponentUpdateBatch::Scope updates(batch);
    window = Window::create(); window->setBounds({0, 0, 960, 680}); window->setMinimumSize({560, 360});
    window->setTitle("Navigator"); window->onClose([] { g_exit(0); }); FillLayout::create(window);
    root = panel(window, background); FillLayout::create(root);
    auto column = panel(root, background); auto frame = flex(column, Orientation::Vertical);
    menuBar = panel(column, _RGB(250, 251, 253)); part(frame, menuBar, menuTop);
    auto menuLayout = StackLayout::create(menuBar); menuLayout->setOrientation(Orientation::Horizontal);
    menuLayout->setPadding({1, 16, 1, 0}); menuLayout->setSpace(2);
    toolbar = panel(column, _RGB(250, 251, 253)); part(frame, toolbar, 58);
    auto tools = flex(toolbar, Orientation::Horizontal, {11, 12, 11, 12}, 12);
    auto nav = panel(toolbar); auto navLayout = flex(nav, Orientation::Horizontal, {2, 0, 2, 0}, 2); part(tools, nav, 66);
    backButton = iconControl(nav, Icon::Back, "Back  (Alt+Left)", Action::Back); part(navLayout, backButton, 32);
    forwardButton = iconControl(nav, Icon::Forward, "Forward  (Alt+Right)", Action::Forward); part(navLayout, forwardButton, 32);
    location = TextBox::create(); toolbar->addChild(location); part(tools, location, 120, 1, 1);
    location->setNumericProperty(ComponentProperty::Icon, uint32_t(Icon::Folder));
    location->setStringProperty(ComponentProperty::Tooltip, "Enter a path  (Ctrl+L)");
    location->addKeyListener([](KeyEvent& event) { handleKey(event, true); if(event.info.pressed && event.info.scancode == 28) request(Action::Path, location->getTitle()); });
    auto upHost = panel(toolbar); FillLayout::create(upHost)->setPadding({2, 0, 2, 0}); part(tools, upHost, 32);
    upButton = iconControl(upHost, Icon::Up, "Parent folder  (Alt+Up)", Action::Up);
    auto refreshHost = panel(toolbar); FillLayout::create(refreshHost)->setPadding({2, 0, 2, 0}); part(tools, refreshHost, 32);
    refreshButton = iconControl(refreshHost, Icon::Refresh, "Refresh  (F5)", Action::Refresh);
    auto filter = panel(toolbar); auto filterLayout = flex(filter, Orientation::Horizontal, {}, 4); part(tools, filter, 200, 0, 1);
    filterBox = TextBox::create(); filter->addChild(filterBox); part(filterLayout, filterBox, 0, 1, 1);
    filterBox->setNumericProperty(ComponentProperty::Icon, uint32_t(Icon::Search));
    filterBox->setStringProperty(ComponentProperty::Placeholder, "Filter this folder");
    filterBox->setStringProperty(ComponentProperty::Tooltip, "Filter by name  (Ctrl+F)");
    filterBox->addTitleListener([](std::string title) { request(Action::Filter, std::move(title)); });
    filterBox->addKeyListener([](KeyEvent& event) { handleKey(event, true); if(event.info.pressed) request(Action::Filter, filterBox->getTitle()); });
    clearButton = iconControl(filter, Icon::Close, "Clear filter", Action::ClearFilter); part(filterLayout, clearButton, 30);
    auto body = panel(column, background); auto bodyLayout = flex(body, Orientation::Horizontal); part(frame, body, 0, 1, 1);
    sidebar = panel(body, _RGB(247, 249, 252)); part(bodyLayout, sidebar, 176);
    sidebar->setVisible(showFavorites);
    auto side = flex(sidebar, Orientation::Vertical, {14, 8, 4, 8});
    auto sideText = [&](const std::string& text, int size, ColorArgb color, int height) {
        auto host = panel(sidebar); FillLayout::create(host)->setPadding({0, 12, 0, 0}); part(side, host, height); return label(host, text, color, size);
    };
    sideText("FAVORITES", 11, _RGB(145, 155, 173), 22); gap(sidebar, side, 8);
    auto placeList = panel(sidebar); auto placeLayout = StackLayout::create(placeList); placeLayout->setOrientation(Orientation::Vertical); placeLayout->setSpace(4); part(side, placeList, 186);
    const char* names[] = {"Home", "Desktop", "Applications", "Computer", "System"};
    const char* paths[] = {"/user", "/user/desktop", "/applications", "/", "/system"};
    const Icon icons[] = {Icon::Home, Icon::Desktop, Icon::Applications, Icon::Computer, Icon::Folder};
    for(int i = 0; i < 5; ++i) {
        auto place = button(placeList, names[i], Action::Path, paths[i]); place->setIcon(icons[i]); sized(place, -1, 34);
        place->setNumericProperty(ComponentProperty::Alignment, uint32_t(TextAlignment::LEFT)); places.push_back({place, paths[i]});
    }
    gap(sidebar, side, 14); sideText("QUICK ACCESS", 11, _RGB(145, 155, 173), 22);
    sideText("Ctrl+L   Go to a folder", 12, _RGB(127, 139, 158), 24); sideText("Ctrl+F   Find a file", 12, _RGB(127, 139, 158), 24); gap(sidebar, side, 0, 1);
    sidebarRule = panel(body, _RGB(233, 237, 244)); part(bodyLayout, sidebarRule, 1);
    sidebarRule->setVisible(showFavorites);
    auto files = panel(body, background); auto filesLayout = flex(files, Orientation::Vertical); part(bodyLayout, files, 0, 1, 1);
    listHeader = panel(files, _RGB(249, 250, 253)); auto headers = flex(listHeader, Orientation::Horizontal, {0, 24, 0, 39}, 10); part(filesLayout, listHeader, 24);
    gap(listHeader, headers, 24); listName = label(listHeader, "Name", _RGB(137, 148, 167), 12); part(headers, listName, 0, 1, 1);
    listKind = label(listHeader, "Kind", _RGB(137, 148, 167), 12); part(headers, listKind, 136);
    fileBackground = panel(files, background); FillLayout::create(fileBackground); part(filesLayout, fileBackground, 0, 1, 1);
    scroller = ScrollPane::create(); scroller->setFixed(true, false); scroller->setVisible(false); fileBackground->addChild(scroller);
    content = Panel::create(); content->setBackground(ARGB(0, 0, 0, 0)); content->setFocusable(true); configureFiles();
    content->addKeyListener([](KeyEvent& event) { handleKey(event); });
    content->addMouseListener([](ComponentMouseEvent* event) { if(event->type == MouseEventType::Press) request(Action::Deselect); });
    content->setBoundsListener([](Rectangle bounds) { request(Action::Resize, "", 1, bounds); }); scroller->setContent(content);
    emptyState = label(fileBackground, "This folder is empty", _RGB(145, 155, 173)); emptyState->setAlignment(TextAlignment::CENTER); emptyState->setVisible(false);
    loadingState = label(fileBackground, "Loading...", _RGB(145, 155, 173)); loadingState->setAlignment(TextAlignment::CENTER);
    auto footer = panel(column); auto footerLayout = flex(footer, Orientation::Horizontal, {2, 16, 2, 8}, 8); part(frame, footer, 28);
    status = label(footer, "", _RGB(137, 148, 167), 12); part(footerLayout, status, 0, 1, 1);
    auto views = panel(footer); auto viewLayout = flex(views, Orientation::Horizontal, {}, 2); part(footerLayout, views, 50);
    iconButton = iconControl(views, Icon::Grid, "Icon view", Action::Icons); part(viewLayout, iconButton, 24);
    listButton = iconControl(views, Icon::List, "List view", Action::List); part(viewLayout, listButton, 24);
    auto overlay = panel(root); FillLayout::create(overlay)->setPadding({menuTop, 0, 0, 0});
    scrim = panel(overlay); scrim->setVisible(false); scrim->setFocusable(true);
    scrim->addMouseListener([](ComponentMouseEvent* event) { if(event->type == MouseEventType::Press) request(Action::Dismiss); });
    scrim->addKeyListener([](KeyEvent& event) { handleKey(event); });
    for(int i = 0; i < 3; ++i) {
        Menu menu; menu.x = 16 + i * 54;
        menu.button = button(menuBar, i == 0 ? "File" : i == 1 ? "View" : "Go", Action::Menu, std::to_string(i)); sized(menu.button, 52, 28);
        menu.panel = panel(scrim); menu.panel->setVisible(false); surface(menu.panel, background, 10, _RGB(222, 228, 238));
        auto rows = StackLayout::create(menu.panel); rows->setOrientation(Orientation::Vertical); rows->setPadding({6, 6, 6, 6}); rows->setSpace(2);
        menu.panel->setFocusable(true); menu.panel->addKeyListener([](KeyEvent& event) { handleKey(event); }); menus.push_back(std::move(menu));
    }
	addMenuItem(menus[0], "New window", Action::NewWindow, "Ctrl+N");
	addMenuItem(menus[0], "Open", Action::Open, "Enter");
	addMenuItem(menus[0], "Open in Editor", Action::Edit);
	addMenuItem(menus[0], "Open terminal here", Action::Terminal);
	addMenuItem(menus[0], "Properties", Action::Properties);
	addMenuItem(menus[0], "Close window", Action::Close, "Ctrl+W");
	addMenuItem(menus[1], "Icon view", Action::Icons);
	addMenuItem(menus[1], "List view", Action::List);
	addMenuItem(menus[1], "Sort by name", Action::SortName);
	addMenuItem(menus[1], "Sort by type", Action::SortType);
	addMenuItem(menus[1], "Reverse order", Action::Reverse);
	addMenuItem(menus[1], "Show hidden files", Action::Hidden, "Ctrl+H");
	addMenuItem(menus[1], "Show favorites", Action::Favorites);
	addMenuItem(menus[1], "Refresh", Action::Refresh, "F5");
	addMenuItem(menus[2], "Back", Action::Back, "Alt+Left");
	addMenuItem(menus[2], "Forward", Action::Forward, "Alt+Right");
	addMenuItem(menus[2], "Parent folder", Action::Up, "Alt+Up");
	addMenuItem(menus[2], "Home", Action::Path, "", "/user");
	addMenuItem(menus[2], "Desktop", Action::Path, "", "/user/desktop");
	addMenuItem(menus[2], "Computer", Action::Path, "", "/");
	addMenuItem(menus[2], "Enter location", Action::Location, "Ctrl+L");
    properties = Window::create(); properties->setShowInTaskbar(false); properties->setTitle("File information"); properties->setResizable(false);
    auto info = flex(properties, Orientation::Vertical, {16, 24, 16, 24}, 4);
    auto eyebrow = label(properties, "FILE INFORMATION", _RGB(145, 155, 173), 11); part(info, eyebrow, 24);
    for(int i = 0; i < 4; ++i) {
        auto line = label(properties, "", i == 0 ? _RGB(35, 44, 65) : mutedColor, i == 0 ? 20 : 13);
        part(info, line, 32); propertyLabels.push_back(line);
    }
    gap(properties, info, 0, 1); auto actions = panel(properties); auto buttons = flex(actions, Orientation::Horizontal); part(info, actions, 34);
    gap(actions, buttons, 0, 1); auto close = button(actions, "Done", Action::Dismiss); close->setVariant(ButtonVariant::Default); part(buttons, close, 90);
    properties->onClose([] { request(Action::Dismiss); }); properties->addKeyListener([](KeyEvent& event) { handleKey(event); });
    dismiss(); root->setBoundsListener([](Rectangle bounds) { request(Action::Resize, "", 0, bounds); });
    window->addKeyListener([](KeyEvent& event) { handleKey(event); });
    window->addFocusListener([](bool focused) { if(!focused) request(Action::Dismiss, "", 1); });
    if(!batch.commit()) { klog("navigator: failed to build UI"); g_exit(1); }
}

void process(Request item)
{
	if(item.action == Action::Resize) {
        if(item.value == 1) {
            if(item.bounds.width != contentWidth) { contentWidth = item.bounds.width; renderEntries(); }
        } else layout(item.bounds);
        return;
    }
	if(item.action == Action::Dismiss && item.value == 1 && propertyOpen) return;
	if(item.action == Action::Menu)
	{
		if(item.value == 0) { toggleMenu(item.text[0] - '0'); return; }
		auto action = static_cast<Action>(-2 - item.value);
		if(openMenu < 0) return;
		bool enabled = false;
		for(auto& menuItem : menus[openMenu].items)
			if(menuItem->action == action) enabled |= menuItem->enabled;
		if(!enabled) return;
		item.action = action;
	}
	if(item.action == Action::MenuHover)
	{
		if(openMenu < 0 || item.text != std::to_string(openMenu)) return;
		menuSelection = item.value;
		for(size_t i = 0; i < menus[openMenu].items.size(); ++i)
			menus[openMenu].items[i]->row->setNumericProperty(ComponentProperty::Selected, false);
		return;
	}
	if(openMenu >= 0 && item.action == Action::MoveSelection)
	{
		if(item.value == 75 || item.value == 77)
			toggleMenu((openMenu + (item.value == 75 ? 2 : 1)) % 3);
		else
		{
			auto& items = menus[openMenu].items;
			int step = item.value == 72 ? -1 : 1;
			for(size_t i = 0; i < items.size(); ++i)
			{
				menuSelection = menuSelection < 0 ? (step > 0 ? 0 : static_cast<int>(items.size()) - 1) :
					(menuSelection + step + items.size()) % items.size();
				if(items[menuSelection]->enabled) break;
			}
			for(size_t i = 0; i < items.size(); ++i)
				items[i]->row->setNumericProperty(ComponentProperty::Selected, static_cast<int>(i) == menuSelection);
		}
		return;
	}
	if(openMenu >= 0 && item.action == Action::Open && item.text.empty())
	{
		if(menuSelection < 0) return;
		auto& selected = menus[openMenu].items[menuSelection];
		if(!selected->enabled) return;
		item.action = selected->action; item.text = selected->target;
	}
	if(propertyOpen && item.action == Action::Open) item.action = Action::Dismiss;
	if(item.action != Action::Hover && item.action != Action::Unhover && item.action != Action::Filter)
	{
		bool restoreFocus = (openMenu >= 0 || propertyOpen) && !(item.action == Action::Dismiss && item.value == 1);
		dismiss();
		if(restoreFocus) focus(content);
	}
	switch(item.action)
	{
		case Action::Path: if(load(item.text)) focus(content); break;
		case Action::Back: if(history.canBack() && load(history.paths[history.position - 1], history.position - 1)) focus(content); break;
		case Action::Forward: if(history.canForward() && load(history.paths[history.position + 1], history.position + 1)) focus(content); break;
		case Action::Up: if(load(navigator::resolvePath(currentPath, ".."))) focus(content); break;
		case Action::Refresh: load(currentPath); break;
		case Action::Filter: if(query != item.text) { query = item.text; renderEntries(); } break;
		case Action::ClearFilter: query.clear(); filterBox->setTitle(""); renderEntries(); focus(filterBox); break;
		case Action::Icons: if(listView) { listView = false; renderEntries(); } focus(content); break;
		case Action::List: if(!listView) { listView = true; renderEntries(); } focus(content); break;
		case Action::SortName: sortType = false; renderEntries(); break;
		case Action::SortType: sortType = true; renderEntries(); break;
		case Action::Reverse: descending = !descending; renderEntries(); break;
		case Action::Hidden: showHidden = !showHidden; renderEntries(); break;
		case Action::Favorites:
		{
			showFavorites = !showFavorites;
			ComponentUpdateBatch batch;
			ComponentUpdateBatch::Scope updates(batch);
			sidebar->setVisible(showFavorites); sidebarRule->setVisible(showFavorites);
			if(!batch.commit()) { klog("navigator: failed to toggle favorites"); g_exit(1); }
			break;
		}
		case Action::Open: if(item.text.empty() || item.text == selectedPath) openSelection(false); break;
		case Action::Edit: openSelection(true); break;
		case Action::Terminal: spawn("/applications/terminal.bin"); break;
		case Action::NewWindow: spawn("/applications/navigator.bin", navigator::argument(currentPath)); break;
		case Action::OpenFolderWindow:
			if(item.text == selectedPath && selection() && selection()->directory)
				spawn("/applications/navigator.bin", navigator::argument(item.text));
			break;
		case Action::Properties: showProperties(); break;
		case Action::Close: g_exit(0); break;
		case Action::Location: focus(location); location->setNumericProperty(ComponentProperty::SelectAll, true); break;
		case Action::Find: focus(filterBox); filterBox->setNumericProperty(ComponentProperty::SelectAll, true); break;
		case Action::Select:
			for(auto& tile : tiles) if(tile->active && tile->path == item.text) { select(item.text); break; }
			break;
		case Action::Deselect: select(""); focus(content); break;
		case Action::MoveSelection:
		{
			if(visible.empty()) break;
			int index = -1;
			for(size_t i = 0; i < visible.size(); ++i)
				if(navigator::resolvePath(currentPath, visible[i].name) == selectedPath) index = i;
			int step = item.value == 75 ? -1 : item.value == 77 ? 1 : item.value == 72 ? -columns : columns;
			index = index < 0 ? 0 : std::max(0, std::min(static_cast<int>(visible.size()) - 1, index + step));
			select(navigator::resolvePath(currentPath, visible[index].name));
			uint32_t offset = 0;
			scroller->getNumericProperty(ComponentProperty::ScrollOffsetY, &offset);
			auto bounds = tiles[index]->panel->getBounds();
			int top = bounds.y;
			int bottom = top + bounds.height, height = scroller->getBounds().height;
			if(top < static_cast<int>(offset)) scroller->setNumericProperty(ComponentProperty::ScrollOffsetY, top);
			else if(bottom > static_cast<int>(offset) + height) scroller->setNumericProperty(ComponentProperty::ScrollOffsetY, std::max(0, bottom - height));
			break;
		}
		case Action::Dismiss: location->setTitle(currentPath); if(item.value == 0) focus(content); break;
		default: break;
	}
}
}

int main(int argc, char* argv[])
{
	g_mutex_acquire(requestReady);
	if(Application::open() != ApplicationOpenStatus::Success) { klog("navigator: failed to create UI"); return -1; }
	buildUi();
	window->setVisible(true);
	load(argc > 1 ? argv[1] : "/");
	focus(content);
	for(;;)
	{
		g_mutex_acquire(requestReady);
		for(;;)
		{
			g_mutex_acquire(requestLock);
			if(requests.empty()) { g_mutex_release(requestLock); break; }
			auto item = std::move(requests.front()); requests.pop_front();
			g_mutex_release(requestLock);
			process(std::move(item));
		}
	}
}
