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

#include "browser_window.hpp"
#include <functional>
#include <libfenster/application.hpp>
#include <libfenster/component_registry.hpp>
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
#include <memory>
#include <stdexcept>
#include "browser_model.hpp"

using namespace fenster;
using navigator::Entry;

namespace
{
	struct BrowserLock
	{
		g_user_mutex mutex;
		explicit BrowserLock(g_user_mutex mutex) : mutex(mutex) { g_mutex_acquire(mutex); }
		~BrowserLock() { g_mutex_release(mutex); }
	};

	const auto background = _RGB(255, 255, 255);
	const auto selectedColor = _RGB(233, 237, 255);
	const auto hoverColor = _RGB(244, 246, 251);
	const auto mutedColor = _RGB(102, 115, 135);
	constexpr int fileMargin = 12, gridTop = 8, gridStepX = 132, gridStepY = 124, gridHeight = 116;
	constexpr int listTop = 4, listStep = 42, listHeight = 40;
	constexpr int menuTop = 30, menuInset = 6, menuStep = 32;

	enum class Action
	{
		Path,
		Back,
		Forward,
		Up,
		Refresh,
		Filter,
		ClearFilter,
		Icons,
		List,
		SortName,
		SortType,
		Reverse,
		Hidden,
		Favorites,
		Open,
		Edit,
		Terminal,
		NewWindow,
		OpenFolderWindow,
		Properties,
		Close,
		Location,
		Find,
		Select,
		Hover,
		Unhover,
		Deselect,
		MoveSelection,
		Menu,
		MenuHover,
		Dismiss,
		Resize
	};
	struct Request
	{
		Action action;
		std::string text;
		int value = 0;
		Rectangle bounds;
	};
	void navigatorReapWindows();

	struct BrowserWindow : std::enable_shared_from_this<BrowserWindow>
	{
		std::vector<Component*> components;
		std::vector<std::unique_ptr<Layout>> layouts;
		g_user_mutex tilesLock = g_mutex_initialize();
		bool closed = false;

		~BrowserWindow() { g_mutex_destroy(tilesLock); }

		template<class T>
		T* browserComponent()
		{
			auto component = T::create();
			if(!component)
				throw std::runtime_error("Could not create browser component");
			components.push_back(component);
			return component;
		}

		template<class T>
		T* browserLayout(Component* component)
		{
			auto layout = T::create(component);
			layouts.emplace_back(layout);
			return layout;
		}

		void browserClose()
		{
			if(closed)
				return;
			closed = true;
			if(window)
				window->destroy();
			if(properties)
				properties->destroy();
			ComponentRegistry::dispose(components);
			components.clear();
			layouts.clear();
		}

		void browserOpen(const std::string& path)
		{
			browserBuildUi();
			window->setVisible(true);
			browserLoad(path);
			browserFocus(window);
			browserFocus(content);
		}

		Window* window = nullptr;
		Panel *root, *menuBar, *sidebar, *fileBackground, *content, *scrim;
		Window* properties = nullptr;
		Layout* contentLayout = nullptr;
		bool contentList = false;
		ScrollPane* scroller;
		TextBox *location, *filterBox;
		Label *status, *emptyState, *loadingState, *listName, *listKind;
		Panel *toolbar, *listHeader, *sidebarRule;
		Button *refreshButton, *clearButton;
		Button *backButton, *forwardButton, *upButton, *iconButton, *listButton;
		std::string currentPath = "/", query, selectedPath, loadError;
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

		void browserRequest(Action action, std::string text = "", int value = 0, Rectangle bounds = {})
		{
			auto self = shared_from_this();
			navigatorEnqueue(
					[self, action, text = std::move(text), value, bounds]() mutable
					{
						if(self->closed)
							return;
						try
						{
							self->browserProcess({action, std::move(text), value, bounds});
						}
						catch(const std::exception& error)
						{
							klog("navigator: %s", error.what());
							self->browserClose();
						}
						navigatorReapWindows();
					},
					this, action == Action::Filter || action == Action::Resize ? int(action) * 2 + value : -1);
		}

		void browserFocus(Component* component)
		{
			CommandFocusRequest message;
			message.header.id = ProtocolCommandId::Focus;
			message.id = component->getId();
			auto tx = platformCreateMessageTransaction();
			if(!sendUiMessage(DelegateTaskId, &message, sizeof(message), tx))
			{
				klog("navigator: failed to focus component");
				throw std::runtime_error("Browser UI communication failed");
			}
			uint8_t response[SYS_MESSAGE_HEADER_SIZE + sizeof(CommandFocusResponse)];
			platformReceiveMessage(response, sizeof(response), tx);
		}

		void browserHandleKey(KeyEvent& event, bool textInput = false)
		{
			const auto& key = event.info;
			if(!key.pressed)
				return;
			auto scan = key.scancode & 0xff;
			if(scan == 1)
				browserRequest(Action::Dismiss);
			else if(key.ctrl && scan == 38)
				browserRequest(Action::Location); // Ctrl+L
			else if(key.ctrl && scan == 33)
				browserRequest(Action::Find); // Ctrl+F
			else if(key.ctrl && scan == 49)
				browserRequest(Action::NewWindow); // Ctrl+N
			else if(key.ctrl && scan == 17)
				browserRequest(Action::Close); // Ctrl+W
			else if(key.ctrl && scan == 35)
				browserRequest(Action::Hidden); // Ctrl+H
			else if(key.ctrl && scan == 19)
				browserRequest(Action::Refresh); // Ctrl+R
			else if(key.alt && scan == 75)
				browserRequest(Action::Back);
			else if(key.alt && scan == 77)
				browserRequest(Action::Forward);
			else if(key.alt && scan == 72)
				browserRequest(Action::Up);
			else if(scan == 63)
				browserRequest(Action::Refresh); // F5
			else if(!textInput && scan == 14)
				browserRequest(Action::Up);
			else if(!textInput && scan == 28)
				browserRequest(Action::Open);
			else if(!textInput && (scan == 75 || scan == 77 || scan == 72 || scan == 80))
				browserRequest(Action::MoveSelection, "", scan);
		}

		Label* browserLabel(Component* parent, const std::string& title, ColorArgb color = _RGB(30, 42, 60),
							int size = 14)
		{
			auto result = browserComponent<Label>();
			result->setTitle(title);
			result->setColor(color);
			if(size != 14)
				result->setNumericProperty(ComponentProperty::FontSize, size);
			parent->addChild(result);
			return result;
		}

		Button* browserButton(Component* parent, const std::string& title, Action action, const std::string& text = "")
		{
			auto result = browserComponent<Button>();
			result->setTitle(title);
			result->setVariant(ButtonVariant::Ghost);
			result->setActionListener([=]() { browserRequest(action, text); });
			result->addKeyListener([this](KeyEvent& event) { browserHandleKey(event); });
			parent->addChild(result);
			return result;
		}

		Button* browserIconControl(Component* parent, Icon icon, const std::string& hint, Action action)
		{
			auto control = browserButton(parent, "", action);
			control->setIcon(icon);
			control->setTooltip(hint);
			return control;
		}

		void browserSurface(Panel* panel, ColorArgb color, int radius = 0, ColorArgb border = 0)
		{
			panel->setBackground(color);
			panel->setNumericProperty(ComponentProperty::CornerRadius, radius);
			panel->setNumericProperty(ComponentProperty::BorderColor, border);
		}

		void browserSized(Component* component, int width, int height)
		{
			component->setMinimumSize({std::max(0, width), std::max(0, height)});
			component->setMaximumSize({width, height});
		}
		Panel* browserPanel(Component* parent, ColorArgb color = ARGB(0, 0, 0, 0))
		{
			auto result = browserComponent<Panel>();
			result->setBackground(color);
			parent->addChild(result);
			return result;
		}
		FlexLayout* browserFlex(Component* component, Orientation orientation, Insets padding = {}, int space = 0)
		{
			auto result = browserLayout<FlexLayout>(component);
			result->setOrientation(orientation);
			result->setPadding(padding);
			result->setSpace(space);
			return result;
		}
		void browserPart(FlexLayout* layout, Component* child, int basis, float grow = 0, float shrink = 0)
		{
			layout->setComponentInfo(child, grow, shrink, basis);
		}
		void browserGap(Component* parent, FlexLayout* layout, int size, float grow = 0)
		{
			browserPart(layout, browserPanel(parent), size, grow);
		}
		void browserConfigureTile(Tile& tile)
		{
			if(tile.mode == int(listView))
				return;
			tile.mode = listView;
			if(!tile.layout)
			{
				tile.layout = browserLayout<FlexLayout>(tile.panel);
				tile.imageLayout = browserLayout<FillLayout>(tile.imageHost);
			}
			browserSized(tile.panel, listView ? -1 : 128, listView ? listHeight : gridHeight);
			tile.layout->setOrientation(listView ? Orientation::Horizontal : Orientation::Vertical);
			tile.layout->setPadding(listView ? Insets(0, 12, 0, 12) : Insets(4, 6, 6, 6));
			tile.layout->setSpace(listView ? 10 : 2);
			browserPart(tile.layout, tile.imageHost, listView ? 24 : 58);
			browserPart(tile.layout, tile.label, listView ? 0 : 25, listView ? 1 : 0, listView ? 1 : 0);
			browserPart(tile.layout, tile.type, listView ? 136 : 18);
			tile.imageLayout->setPadding(listView ? Insets(8, 0, 8, 0) : Insets(6, 32, 0, 32));
		}
		void browserConfigureFiles()
		{
			if(contentLayout && contentList == listView)
				return;
			layouts.erase(std::remove_if(layouts.begin(), layouts.end(),
										 [this](const auto& layout) { return layout.get() == contentLayout; }),
						  layouts.end());
			contentList = listView;
			if(listView)
			{
				auto layout = browserLayout<StackLayout>(content);
				layout->setOrientation(Orientation::Vertical);
				layout->setPadding({listTop, fileMargin, 12, fileMargin});
				layout->setSpace(listStep - listHeight);
				contentLayout = layout;
			}
			else
			{
				auto layout = browserLayout<FlowLayout>(content);
				layout->setPadding({gridTop, fileMargin, 8, fileMargin});
				layout->setSpace(4);
				content->setNumericProperty(ComponentProperty::LayoutSpaceVertical, gridStepY - gridHeight);
				contentLayout = layout;
			}
		}

		void browserDismiss()
		{
			if(openMenu < 0 && !propertyOpen)
				return;
			for(auto& menu: menus)
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

		const Entry* browserSelection()
		{
			for(const auto& entry: visible)
				if(navigator::resolvePath(currentPath, entry.name) == selectedPath)
					return &entry;
			return nullptr;
		}

		void browserUpdateStatus()
		{
			if(!loadError.empty())
			{
				status->setColor(_RGB(180, 50, 45));
				status->setTitle(loadError);
				return;
			}
			auto entry = browserSelection();
			std::string text = std::to_string(visible.size()) + (visible.size() == 1 ? " item" : " items");
			if(!query.empty())
				text += " matching \"" + query + "\"";
			if(entry)
				text += "  |  " + entry->name + "  -  " + navigator::typeName(*entry);
			status->setColor(mutedColor);
			status->setTitle(text);
		}

		void browserSelect(const std::string& path)
		{
			if(selectedPath == path)
				return;
			selectedPath = path;
			ComponentUpdateBatch batch;
			ComponentUpdateBatch::Scope updates(batch);
			for(auto& tile: tiles)
				if(tile->active)
				{
					auto selected = tile->path == path;
					if(tile->selected != selected)
					{
						tile->panel->setNumericProperty(ComponentProperty::Selected, selected);
						tile->selected = selected;
					}
				}
			browserUpdateStatus();
			if(!batch.commit())
			{
				klog("navigator: failed to update selection");
				throw std::runtime_error("Browser UI communication failed");
			}
		}

		bool browserEnsureTiles(size_t count, ComponentUpdateBatch& batch)
		{
			if(count <= tiles.size())
				return true;
			auto missing = count - tiles.size();
			auto panels = Component::createComponents<Panel, ComponentType::Panel>(missing * 2);
			components.insert(components.end(), panels.begin(), panels.end());
			auto images = Component::createComponents<Image, ComponentType::Image>(missing);
			components.insert(components.end(), images.begin(), images.end());
			auto labels = Component::createComponents<Label, ComponentType::Label>(missing * 2);
			components.insert(components.end(), labels.begin(), labels.end());
			if(panels.size() != missing * 2 || images.size() != missing || labels.size() != missing * 2)
				return false;
			for(size_t i = 0; i < missing; ++i)
			{
				auto tile = std::make_unique<Tile>();
				tile->panel = panels[i * 2];
				tile->imageHost = panels[i * 2 + 1];
				tile->image = images[i];
				tile->label = labels[i * 2];
				tile->type = labels[i * 2 + 1];
				auto stable = tile.get();
				batch.setNumericProperty(tile->panel, ComponentProperty::Focusable, true);
				batch.addMouseListener(tile->panel,
									   [this, stable](ComponentMouseEvent* event)
									   {
										   std::string path;
										   {
											   BrowserLock tileGuard(tilesLock);
											   path = stable->active ? stable->path : "";
										   }

										   if(path.empty())
											   return;
										   if(event->type == MouseEventType::Press &&
											  (event->buttons & MouseButton::Button1))
										   {
											   browserRequest(Action::Select, path);
											   if(event->ctrl)
												   browserRequest(Action::OpenFolderWindow, path);
											   else if(event->clickCount == 2)
												   browserRequest(Action::Open, path);
										   }
									   });
				tile->panel->addKeyListener([this](KeyEvent& event) { browserHandleKey(event); });
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

		void browserRenderEntries(bool openingFolder = false)
		{
			ComponentUpdateBatch batch;
			ComponentUpdateBatch::Scope updates(batch);
			visible = navigator::visibleEntries(entries, query, showHidden, sortType, descending);
			if(!browserSelection())
				selectedPath.clear();
			browserConfigureFiles();
			columns = listView ? 1 : std::max(1, (contentWidth - 2 * fileMargin + 4) / gridStepX);
			content->setVisible(false);
			if(!browserEnsureTiles(visible.size(), batch))
			{
				klog("navigator: failed to create file tiles");
				throw std::runtime_error("Browser UI communication failed");
			}
			for(size_t i = 0; i < tiles.size(); ++i)
			{
				auto& tile = *tiles[i];
				if(i >= visible.size())
				{
					if(tile.active)
						batch.setNumericProperty(tile.panel, ComponentProperty::Visible, false);
					{
						BrowserLock guard(tilesLock);
						tile.active = false;
					}
					continue;
				}
				const auto& entry = visible[i];
				const auto path = navigator::resolvePath(currentPath, entry.name);
				const auto kind = navigator::typeName(entry);
				const auto icon = entry.directory		  ? Icon::Folder
								  : kind == "Application" ? Icon::Applications
														  : Icon::Document;
				const int width = std::max(80, contentWidth - 2 * fileMargin);
				const auto title = navigator::displayName(entry.name, listView ? std::max(8, (width - 200) / 8) : 14);
				const bool selected = path == selectedPath;
				if(tile.mode != int(listView))
				{
					auto alignment = uint32_t(listView ? TextAlignment::LEFT : TextAlignment::CENTER);
					batch.setNumericProperty(tile.label, ComponentProperty::Alignment, alignment);
					batch.setNumericProperty(tile.type, ComponentProperty::Alignment, alignment);
				}
				if(tile.title != title)
				{
					batch.setStringProperty(tile.label, ComponentProperty::Title, title);
					tile.title = title;
				}
				if(tile.name != entry.name)
				{
					batch.setStringProperty(tile.panel, ComponentProperty::Tooltip, entry.name);
					tile.name = entry.name;
				}
				if(tile.kind != kind)
				{
					batch.setStringProperty(tile.type, ComponentProperty::Title, kind);
					tile.kind = kind;
				}
				if(tile.icon != icon)
				{
					batch.setNumericProperty(tile.image, ComponentProperty::Icon, uint32_t(icon));
					tile.icon = icon;
				}
				if(tile.selected != selected)
				{
					batch.setNumericProperty(tile.panel, ComponentProperty::Selected, selected);
					tile.selected = selected;
				}
				if(!tile.active)
					batch.setNumericProperty(tile.panel, ComponentProperty::Visible, true);
				{
					BrowserLock guard(tilesLock);
					tile.path = path;
					tile.active = true;
				}
			}
			for(auto& tile: tiles)
				if(tile->active)
					browserConfigureTile(*tile);
			if(!batch.commit())
			{
				klog("navigator: failed to update file tiles");
				throw std::runtime_error("Browser UI communication failed");
			}
			content->setVisible(true);
			emptyState->setTitle(query.empty() ? "This folder is empty" : "No matching items. Try a different filter.");
			iconButton->setSelected(!listView);
			listButton->setSelected(listView);
			listHeader->setVisible(listView);
			clearButton->setVisible(!query.empty());
			browserUpdateStatus();
			if(!batch.commit())
			{
				klog("navigator: failed to finish folder updates");
				throw std::runtime_error("Browser UI communication failed");
			}
			if(openingFolder)
			{
				if(!scroller->prepareForDisplay())
				{
					klog("navigator: failed to prepare folder contents");
					throw std::runtime_error("Browser UI communication failed");
				}
				auto width = content->getBounds().width;
				if(width > 0 && width != contentWidth)
				{
					contentWidth = width;
					browserRenderEntries(true);
					return;
				}
			}
			scroller->setVisible(true);
			emptyState->setVisible(visible.empty());
			loadingState->setVisible(false);
			if(!batch.commit())
			{
				klog("navigator: failed to show folder contents");
				throw std::runtime_error("Browser UI communication failed");
			}
		}

		bool browserLoad(const std::string& target, int historyPosition = -1)
		{
			ComponentUpdateBatch batch;
			ComponentUpdateBatch::Scope updates(batch);
			scroller->setVisible(false);
			emptyState->setVisible(false);
			loadingState->setVisible(true);
			// Hide the previous folder before filesystem work or metadata updates begin.
			if(!batch.commit())
			{
				klog("navigator: failed to hide folder contents");
				throw std::runtime_error("Browser UI communication failed");
			}
			auto fail = [&](const std::string& message)
			{
				loadError = message;
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
									entry->type == G_FS_NODE_TYPE_FOLDER || entry->type == G_FS_NODE_TYPE_ROOT ||
											entry->type == G_FS_NODE_TYPE_MOUNTPOINT,
									entry->type == G_FS_NODE_TYPE_FILE});
			g_close_directory(directory);
			// filesystemReadDirectory currently reports ERROR for a folder with no child list.
			if(readStatus != G_FS_READ_DIRECTORY_EOD && !children.empty())
				return fail("Could not finish reading folder: " + path);
			loadError.clear();
			bool changed = currentPath != canonical;
			currentPath = canonical;
			entries = std::move(children);
			if(historyPosition >= 0)
				history.position = historyPosition;
			else
				history.record(currentPath);
			if(changed)
			{
				selectedPath.clear();
				query.clear();
				filterBox->setTitle("");
				scroller->setNumericProperty(ComponentProperty::ScrollOffsetY, 0);
			}
			location->setTitle(currentPath);
			window->setTitle(currentPath == "/" ? "Computer" : currentPath);
			backButton->setEnabled(history.canBack());
			forwardButton->setEnabled(history.canForward());
			upButton->setEnabled(currentPath != "/");
			for(auto& place: places)
				place.first->setSelected(place.second == currentPath);
			browserRenderEntries(true);
			if(!batch.commit())
			{
				klog("navigator: failed to update folder metadata");
				throw std::runtime_error("Browser UI communication failed");
			}
			return true;
		}

		void browserSpawn(const std::string& program, const std::string& args = "")
		{
			if(g_spawn_poim(program.c_str(), args.c_str(), currentPath.c_str(), G_SECURITY_LEVEL_APPLICATION, nullptr,
							nullptr, nullptr, G_LAUNCH_MODE_UI) != G_SPAWN_STATUS_SUCCESSFUL)
			{
				status->setColor(_RGB(180, 50, 45));
				status->setTitle("Could not open " + program);
			}
		}

		void browserOpenSelection(bool editor)
		{
			auto entry = browserSelection();
			if(!entry)
				return;
			auto path = selectedPath;
			if(entry->directory)
			{
				if(!editor && browserLoad(path))
					browserFocus(content);
			}
			else if(entry->file)
			{
				if(!editor && navigator::typeName(*entry) == "Application")
					browserSpawn(path);
				else
					browserSpawn("/applications/editor.bin", navigator::argument(path));
			}
		}

		void browserShowProperties()
		{
			auto entry = browserSelection();
			if(!entry)
				return;
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
					if(bytes >= 0)
						size = std::to_string(bytes) + " bytes";
				}
			}
			propertyLabels[3]->setTitle(size);
			auto parent = window->getBounds();
			properties->setBounds({parent.x + std::max(0, (parent.width - 500) / 2),
								   parent.y + std::max(0, (parent.height - 320) / 2), 500, 320});
			properties->setVisible(true);
			propertyOpen = true;
			browserFocus(properties);
		}

		void browserToggleMenu(int index)
		{
			bool close = openMenu == index;
			browserDismiss();
			if(close)
			{
				browserFocus(content);
				return;
			}
			openMenu = index;
			auto entry = browserSelection();
			for(auto& item: menus[index].items)
			{
				item->enabled = true;
				std::string prefix;
				switch(item->action)
				{
					case Action::Open:
						item->enabled = entry && (entry->directory || entry->file);
						break;
					case Action::Edit:
						item->enabled = entry && entry->file;
						break;
					case Action::Properties:
						item->enabled = entry != nullptr;
						break;
					case Action::Back:
						item->enabled = history.canBack();
						break;
					case Action::Forward:
						item->enabled = history.canForward();
						break;
					case Action::Up:
						item->enabled = currentPath != "/";
						break;
					case Action::Icons:
						prefix = !listView ? "* " : "  ";
						break;
					case Action::List:
						prefix = listView ? "* " : "  ";
						break;
					case Action::SortName:
						prefix = !sortType ? "* " : "  ";
						break;
					case Action::SortType:
						prefix = sortType ? "* " : "  ";
						break;
					case Action::Reverse:
						prefix = descending ? "* " : "  ";
						break;
					case Action::Hidden:
						prefix = showHidden ? "* " : "  ";
						break;
					case Action::Favorites:
						prefix = showFavorites ? "* " : "  ";
						break;
					default:
						break;
				}
				item->title->setTitle(item->text);
				item->icon->setNumericProperty(ComponentProperty::Icon,
											   static_cast<uint32_t>(prefix == "* " ? Icon::Check : item->defaultIcon));
				item->title->setColor(item->enabled ? _RGB(30, 42, 60) : _RGB(160, 168, 180));
				item->row->setNumericProperty(ComponentProperty::Selected, false);
			}
			menus[index].button->setSelected(true);
			scrim->setVisible(true);
			menus[index].panel->setVisible(true);
			browserFocus(menus[index].panel);
		}

		void browserLayout(Rectangle bounds)
		{
			rootWidth = bounds.width;
			rootHeight = bounds.height;
			for(auto& menu: menus)
			{
				menu.x = menu.button->getBounds().x;
				menu.panel->setBounds({std::min(menu.x, std::max(0, rootWidth - 272)), 0, 272,
									   2 * menuInset + int(menu.items.size()) * menuStep - 2});
			}
		}

		void browserAddMenuItem(Menu& menu, const std::string& text, Action action, const std::string& shortcut = "",
								const std::string& target = "")
		{
			auto item = std::make_unique<MenuItem>();
			item->row = browserComponent<Panel>();
			item->title = browserLabel(item->row, text);
			item->action = action;
			item->text = text;
			item->target = target;
			browserSized(item->row, -1, 30);
			auto row = browserFlex(item->row, Orientation::Horizontal, {0, 10, 0, 10}, 10);
			item->row->setFocusable(true);
			item->row->setNumericProperty(ComponentProperty::CornerRadius, 6);
			item->row->setNumericProperty(ComponentProperty::HoverBackground, _RGB(241, 244, 251));
			item->row->setNumericProperty(ComponentProperty::SelectedBackground, _RGB(233, 237, 255));
			auto iconHost = browserPanel(item->row);
			auto inset = browserLayout<FillLayout>(iconHost);
			inset->setPadding({7, 0, 7, 0});
			item->icon = browserComponent<Image>();
			iconHost->addChild(item->icon);
			// The title was created first; re-adding it places it after the icon host.
			item->row->addChild(item->title);
			browserPart(row, iconHost, 16);
			browserPart(row, item->title, 0, 1, 1);
			if(!shortcut.empty())
			{
				auto hint = browserLabel(item->row, shortcut, mutedColor, 12);
				hint->setAlignment(TextAlignment::RIGHT);
				browserPart(row, hint, 60);
			}

			switch(action)
			{
				case Action::NewWindow:
					item->defaultIcon = Icon::NewWindow;
					break;
				case Action::Open:
					item->defaultIcon = Icon::Folder;
					break;
				case Action::Edit:
					item->defaultIcon = Icon::Document;
					break;
				case Action::Terminal:
					item->defaultIcon = Icon::Terminal;
					break;
				case Action::Properties:
					item->defaultIcon = Icon::Info;
					break;
				case Action::Close:
					item->defaultIcon = Icon::Close;
					break;
				case Action::Back:
					item->defaultIcon = Icon::Back;
					break;
				case Action::Forward:
					item->defaultIcon = Icon::Forward;
					break;
				case Action::Up:
					item->defaultIcon = Icon::Up;
					break;
				case Action::Refresh:
					item->defaultIcon = Icon::Refresh;
					break;
				case Action::Location:
					item->defaultIcon = Icon::Folder;
					break;
				case Action::Path:
					item->defaultIcon = target == "/user"			? Icon::Home
										: target == "/user/desktop" ? Icon::Desktop
																	: Icon::Computer;
					break;
				default:
					break;
			}
			int rowIndex = menu.items.size();
			int menuIndex = &menu - menus.data();
			item->row->addMouseListener(
					[this, rowIndex, menuIndex, action, target](ComponentMouseEvent* event)
					{
						if(event->type == MouseEventType::Press && (event->buttons & MouseButton::Button1))
						{
							// Menu state and enablement are owned by the worker as well.
							browserRequest(Action::Menu, target, -2 - static_cast<int>(action));
						}
						else if(event->type == MouseEventType::Enter)
							browserRequest(Action::MenuHover, std::to_string(menuIndex), rowIndex);
					});
			item->row->addKeyListener([this](KeyEvent& event) { browserHandleKey(event); });
			menu.panel->addChild(item->row);
			menu.items.push_back(std::move(item));
		}

		void browserBuildUi()
		{
			ComponentUpdateBatch batch;
			ComponentUpdateBatch::Scope updates(batch);
			window = browserComponent<Window>();
			window->setBounds({0, 0, 750, 550});
			window->setMinimumSize({400, 250});
			window->setTitle("Navigator");
			window->onClose([this] { browserRequest(Action::Close); });
			browserLayout<FillLayout>(window);
			root = browserPanel(window, background);
			browserLayout<FillLayout>(root);
			auto column = browserPanel(root, background);
			auto frame = browserFlex(column, Orientation::Vertical);
			menuBar = browserPanel(column, _RGB(250, 251, 253));
			browserPart(frame, menuBar, menuTop);
			auto menuLayout = browserLayout<StackLayout>(menuBar);
			menuLayout->setOrientation(Orientation::Horizontal);
			menuLayout->setPadding({1, 16, 1, 0});
			menuLayout->setSpace(2);
			toolbar = browserPanel(column, _RGB(250, 251, 253));
			browserPart(frame, toolbar, 58);
			auto tools = browserFlex(toolbar, Orientation::Horizontal, {11, 12, 11, 12}, 12);
			auto nav = browserPanel(toolbar);
			auto navLayout = browserFlex(nav, Orientation::Horizontal, {2, 0, 2, 0}, 2);
			browserPart(tools, nav, 66);
			backButton = browserIconControl(nav, Icon::Back, "Back  (Alt+Left)", Action::Back);
			browserPart(navLayout, backButton, 32);
			forwardButton = browserIconControl(nav, Icon::Forward, "Forward  (Alt+Right)", Action::Forward);
			browserPart(navLayout, forwardButton, 32);
			location = browserComponent<TextBox>();
			toolbar->addChild(location);
			browserPart(tools, location, 120, 1, 1);
			location->setNumericProperty(ComponentProperty::Icon, uint32_t(Icon::Folder));
			location->setStringProperty(ComponentProperty::Tooltip, "Enter a path  (Ctrl+L)");
			location->addKeyListener(
					[this](KeyEvent& event)
					{
						browserHandleKey(event, true);
						if(event.info.pressed && event.info.scancode == 28)
							browserRequest(Action::Path, location->getTitle());
					});
			auto upHost = browserPanel(toolbar);
			browserLayout<FillLayout>(upHost)->setPadding({2, 0, 2, 0});
			browserPart(tools, upHost, 32);
			upButton = browserIconControl(upHost, Icon::Up, "Parent folder  (Alt+Up)", Action::Up);
			auto refreshHost = browserPanel(toolbar);
			browserLayout<FillLayout>(refreshHost)->setPadding({2, 0, 2, 0});
			browserPart(tools, refreshHost, 32);
			refreshButton = browserIconControl(refreshHost, Icon::Refresh, "Refresh  (F5)", Action::Refresh);
			auto filter = browserPanel(toolbar);
			auto filterLayout = browserFlex(filter, Orientation::Horizontal, {}, 4);
			browserPart(tools, filter, 200, 0, 1);
			filterBox = browserComponent<TextBox>();
			filter->addChild(filterBox);
			browserPart(filterLayout, filterBox, 0, 1, 1);
			filterBox->setNumericProperty(ComponentProperty::Icon, uint32_t(Icon::Search));
			filterBox->setStringProperty(ComponentProperty::Placeholder, "Filter this folder");
			filterBox->setStringProperty(ComponentProperty::Tooltip, "Filter by name  (Ctrl+F)");
			filterBox->addTitleListener([this](std::string title)
										{ browserRequest(Action::Filter, std::move(title)); });
			filterBox->addKeyListener(
					[this](KeyEvent& event)
					{
						browserHandleKey(event, true);
						if(event.info.pressed)
							browserRequest(Action::Filter, filterBox->getTitle());
					});
			clearButton = browserIconControl(filter, Icon::Close, "Clear filter", Action::ClearFilter);
			browserPart(filterLayout, clearButton, 30);
			auto body = browserPanel(column, background);
			auto bodyLayout = browserFlex(body, Orientation::Horizontal);
			browserPart(frame, body, 0, 1, 1);
			sidebar = browserPanel(body, _RGB(247, 249, 252));
			browserPart(bodyLayout, sidebar, 176);
			sidebar->setVisible(showFavorites);
			auto side = browserFlex(sidebar, Orientation::Vertical, {14, 8, 4, 8});
			auto sideText = [&](const std::string& text, int size, ColorArgb color, int height)
			{
				auto host = browserPanel(sidebar);
				browserLayout<FillLayout>(host)->setPadding({0, 12, 0, 0});
				browserPart(side, host, height);
				return browserLabel(host, text, color, size);
			};
			sideText("FAVORITES", 11, _RGB(145, 155, 173), 22);
			browserGap(sidebar, side, 8);
			auto placeList = browserPanel(sidebar);
			auto placeLayout = browserLayout<StackLayout>(placeList);
			placeLayout->setOrientation(Orientation::Vertical);
			placeLayout->setSpace(4);
			browserPart(side, placeList, 186);
			const char* names[] = {"Home", "Desktop", "Applications", "Computer", "System"};
			const char* paths[] = {"/user", "/user/desktop", "/applications", "/", "/system"};
			const Icon icons[] = {Icon::Home, Icon::Desktop, Icon::Applications, Icon::Computer, Icon::Folder};
			for(int i = 0; i < 5; ++i)
			{
				auto place = browserButton(placeList, names[i], Action::Path, paths[i]);
				place->setIcon(icons[i]);
				browserSized(place, -1, 34);
				place->setNumericProperty(ComponentProperty::Alignment, uint32_t(TextAlignment::LEFT));
				places.push_back({place, paths[i]});
			}
			browserGap(sidebar, side, 14);
			sideText("QUICK ACCESS", 11, _RGB(145, 155, 173), 22);
			sideText("Ctrl+L   Go to a folder", 12, _RGB(127, 139, 158), 24);
			sideText("Ctrl+F   Find a file", 12, _RGB(127, 139, 158), 24);
			browserGap(sidebar, side, 0, 1);
			sidebarRule = browserPanel(body, _RGB(233, 237, 244));
			browserPart(bodyLayout, sidebarRule, 1);
			sidebarRule->setVisible(showFavorites);
			auto files = browserPanel(body, background);
			auto filesLayout = browserFlex(files, Orientation::Vertical);
			browserPart(bodyLayout, files, 0, 1, 1);
			listHeader = browserPanel(files, _RGB(249, 250, 253));
			auto headers = browserFlex(listHeader, Orientation::Horizontal, {0, 24, 0, 39}, 10);
			browserPart(filesLayout, listHeader, 24);
			browserGap(listHeader, headers, 24);
			listName = browserLabel(listHeader, "Name", _RGB(137, 148, 167), 12);
			browserPart(headers, listName, 0, 1, 1);
			listKind = browserLabel(listHeader, "Kind", _RGB(137, 148, 167), 12);
			browserPart(headers, listKind, 136);
			fileBackground = browserPanel(files, background);
			browserLayout<FillLayout>(fileBackground);
			browserPart(filesLayout, fileBackground, 0, 1, 1);
			scroller = browserComponent<ScrollPane>();
			scroller->setFixed(true, false);
			scroller->setVisible(false);
			fileBackground->addChild(scroller);
			content = browserComponent<Panel>();
			content->setBackground(ARGB(0, 0, 0, 0));
			content->setFocusable(true);
			browserConfigureFiles();
			content->addKeyListener([this](KeyEvent& event) { browserHandleKey(event); });
			content->addMouseListener(
					[this](ComponentMouseEvent* event)
					{
						if(event->type == MouseEventType::Press)
							browserRequest(Action::Deselect);
					});
			content->setBoundsListener([this](Rectangle bounds) { browserRequest(Action::Resize, "", 1, bounds); });
			scroller->setContent(content);
			emptyState = browserLabel(fileBackground, "This folder is empty", _RGB(145, 155, 173));
			emptyState->setAlignment(TextAlignment::CENTER);
			emptyState->setVisible(false);
			loadingState = browserLabel(fileBackground, "Loading...", _RGB(145, 155, 173));
			loadingState->setAlignment(TextAlignment::CENTER);
			auto footer = browserPanel(column);
			auto footerLayout = browserFlex(footer, Orientation::Horizontal, {2, 16, 2, 8}, 8);
			browserPart(frame, footer, 28);
			status = browserLabel(footer, "", _RGB(137, 148, 167), 12);
			browserPart(footerLayout, status, 0, 1, 1);
			auto views = browserPanel(footer);
			auto viewLayout = browserFlex(views, Orientation::Horizontal, {}, 2);
			browserPart(footerLayout, views, 50);
			iconButton = browserIconControl(views, Icon::Grid, "Icon view", Action::Icons);
			browserPart(viewLayout, iconButton, 24);
			listButton = browserIconControl(views, Icon::List, "List view", Action::List);
			browserPart(viewLayout, listButton, 24);
			auto overlay = browserPanel(root);
			browserLayout<FillLayout>(overlay)->setPadding({menuTop, 0, 0, 0});
			scrim = browserPanel(overlay);
			scrim->setVisible(false);
			scrim->setFocusable(true);
			scrim->addMouseListener(
					[this](ComponentMouseEvent* event)
					{
						if(event->type == MouseEventType::Press)
							browserRequest(Action::Dismiss);
					});
			scrim->addKeyListener([this](KeyEvent& event) { browserHandleKey(event); });
			for(int i = 0; i < 3; ++i)
			{
				Menu menu;
				menu.x = 16 + i * 54;
				menu.button = browserButton(menuBar,
											i == 0	 ? "File"
											: i == 1 ? "View"
													 : "Go",
											Action::Menu, std::to_string(i));
				browserSized(menu.button, 52, 28);
				menu.panel = browserPanel(scrim);
				menu.panel->setVisible(false);
				browserSurface(menu.panel, background, 10, _RGB(222, 228, 238));
				auto rows = browserLayout<StackLayout>(menu.panel);
				rows->setOrientation(Orientation::Vertical);
				rows->setPadding({6, 6, 6, 6});
				rows->setSpace(2);
				menu.panel->setFocusable(true);
				menu.panel->addKeyListener([this](KeyEvent& event) { browserHandleKey(event); });
				menus.push_back(std::move(menu));
			}
			browserAddMenuItem(menus[0], "New window", Action::NewWindow, "Ctrl+N");
			browserAddMenuItem(menus[0], "Open", Action::Open, "Enter");
			browserAddMenuItem(menus[0], "Open in Editor", Action::Edit);
			browserAddMenuItem(menus[0], "Open terminal here", Action::Terminal);
			browserAddMenuItem(menus[0], "Properties", Action::Properties);
			browserAddMenuItem(menus[0], "Close window", Action::Close, "Ctrl+W");
			browserAddMenuItem(menus[1], "Icon view", Action::Icons);
			browserAddMenuItem(menus[1], "List view", Action::List);
			browserAddMenuItem(menus[1], "Sort by name", Action::SortName);
			browserAddMenuItem(menus[1], "Sort by type", Action::SortType);
			browserAddMenuItem(menus[1], "Reverse order", Action::Reverse);
			browserAddMenuItem(menus[1], "Show hidden files", Action::Hidden, "Ctrl+H");
			browserAddMenuItem(menus[1], "Show favorites", Action::Favorites);
			browserAddMenuItem(menus[1], "Refresh", Action::Refresh, "F5");
			browserAddMenuItem(menus[2], "Back", Action::Back, "Alt+Left");
			browserAddMenuItem(menus[2], "Forward", Action::Forward, "Alt+Right");
			browserAddMenuItem(menus[2], "Parent folder", Action::Up, "Alt+Up");
			browserAddMenuItem(menus[2], "Home", Action::Path, "", "/user");
			browserAddMenuItem(menus[2], "Desktop", Action::Path, "", "/user/desktop");
			browserAddMenuItem(menus[2], "Computer", Action::Path, "", "/");
			browserAddMenuItem(menus[2], "Enter location", Action::Location, "Ctrl+L");
			properties = browserComponent<Window>();
			properties->setShowInTaskbar(false);
			properties->setTitle("File information");
			properties->setResizable(false);
			auto info = browserFlex(properties, Orientation::Vertical, {16, 24, 16, 24}, 4);
			auto eyebrow = browserLabel(properties, "FILE INFORMATION", _RGB(145, 155, 173), 11);
			browserPart(info, eyebrow, 24);
			for(int i = 0; i < 4; ++i)
			{
				auto line = browserLabel(properties, "", i == 0 ? _RGB(35, 44, 65) : mutedColor, i == 0 ? 20 : 13);
				browserPart(info, line, 32);
				propertyLabels.push_back(line);
			}
			browserGap(properties, info, 0, 1);
			auto actions = browserPanel(properties);
			auto buttons = browserFlex(actions, Orientation::Horizontal);
			browserPart(info, actions, 34);
			browserGap(actions, buttons, 0, 1);
			auto close = browserButton(actions, "Done", Action::Dismiss);
			close->setVariant(ButtonVariant::Default);
			browserPart(buttons, close, 90);
			properties->onClose([this] { browserRequest(Action::Dismiss); });
			properties->addKeyListener([this](KeyEvent& event) { browserHandleKey(event); });
			browserDismiss();
			root->setBoundsListener([this](Rectangle bounds) { browserRequest(Action::Resize, "", 0, bounds); });
			window->addKeyListener([this](KeyEvent& event) { browserHandleKey(event); });
			window->addFocusListener(
					[this](bool focused)
					{
						if(!focused)
							browserRequest(Action::Dismiss, "", 1);
					});
			if(!batch.commit())
			{
				klog("navigator: failed to build UI");
				throw std::runtime_error("Browser UI communication failed");
			}
		}

		void browserProcess(Request item)
		{
			if(item.action == Action::Resize)
			{
				if(item.value == 1)
				{
					if(item.bounds.width != contentWidth)
					{
						contentWidth = item.bounds.width;
						browserRenderEntries();
					}
				}
				else
					browserLayout(item.bounds);
				return;
			}
			if(item.action == Action::Dismiss && item.value == 1 && propertyOpen)
				return;
			if(item.action == Action::Menu)
			{
				if(item.value == 0)
				{
					browserToggleMenu(item.text[0] - '0');
					return;
				}
				auto action = static_cast<Action>(-2 - item.value);
				if(openMenu < 0)
					return;
				bool enabled = false;
				for(auto& menuItem: menus[openMenu].items)
					if(menuItem->action == action)
						enabled |= menuItem->enabled;
				if(!enabled)
					return;
				item.action = action;
			}
			if(item.action == Action::MenuHover)
			{
				if(openMenu < 0 || item.text != std::to_string(openMenu))
					return;
				menuSelection = item.value;
				for(size_t i = 0; i < menus[openMenu].items.size(); ++i)
					menus[openMenu].items[i]->row->setNumericProperty(ComponentProperty::Selected, false);
				return;
			}
			if(openMenu >= 0 && item.action == Action::MoveSelection)
			{
				if(item.value == 75 || item.value == 77)
					browserToggleMenu((openMenu + (item.value == 75 ? 2 : 1)) % 3);
				else
				{
					auto& items = menus[openMenu].items;
					int step = item.value == 72 ? -1 : 1;
					for(size_t i = 0; i < items.size(); ++i)
					{
						menuSelection = menuSelection < 0 ? (step > 0 ? 0 : static_cast<int>(items.size()) - 1)
														  : (menuSelection + step + items.size()) % items.size();
						if(items[menuSelection]->enabled)
							break;
					}
					for(size_t i = 0; i < items.size(); ++i)
						items[i]->row->setNumericProperty(ComponentProperty::Selected,
														  static_cast<int>(i) == menuSelection);
				}
				return;
			}
			if(openMenu >= 0 && item.action == Action::Open && item.text.empty())
			{
				if(menuSelection < 0)
					return;
				auto& selected = menus[openMenu].items[menuSelection];
				if(!selected->enabled)
					return;
				item.action = selected->action;
				item.text = selected->target;
			}
			if(propertyOpen && item.action == Action::Open)
				item.action = Action::Dismiss;
			if(item.action != Action::Hover && item.action != Action::Unhover && item.action != Action::Filter)
			{
				bool restoreFocus =
						(openMenu >= 0 || propertyOpen) && !(item.action == Action::Dismiss && item.value == 1);
				browserDismiss();
				if(restoreFocus)
					browserFocus(content);
			}
			switch(item.action)
			{
				case Action::Path:
					if(browserLoad(item.text))
						browserFocus(content);
					break;
				case Action::Back:
					if(history.canBack() && browserLoad(history.paths[history.position - 1], history.position - 1))
						browserFocus(content);
					break;
				case Action::Forward:
					if(history.canForward() && browserLoad(history.paths[history.position + 1], history.position + 1))
						browserFocus(content);
					break;
				case Action::Up:
					if(browserLoad(navigator::resolvePath(currentPath, "..")))
						browserFocus(content);
					break;
				case Action::Refresh:
					browserLoad(currentPath);
					break;
				case Action::Filter:
					if(query != item.text)
					{
						query = item.text;
						browserRenderEntries();
					}
					break;
				case Action::ClearFilter:
					query.clear();
					filterBox->setTitle("");
					browserRenderEntries();
					browserFocus(filterBox);
					break;
				case Action::Icons:
					if(listView)
					{
						listView = false;
						browserRenderEntries();
					}
					browserFocus(content);
					break;
				case Action::List:
					if(!listView)
					{
						listView = true;
						browserRenderEntries();
					}
					browserFocus(content);
					break;
				case Action::SortName:
					sortType = false;
					browserRenderEntries();
					break;
				case Action::SortType:
					sortType = true;
					browserRenderEntries();
					break;
				case Action::Reverse:
					descending = !descending;
					browserRenderEntries();
					break;
				case Action::Hidden:
					showHidden = !showHidden;
					browserRenderEntries();
					break;
				case Action::Favorites:
				{
					showFavorites = !showFavorites;
					ComponentUpdateBatch batch;
					ComponentUpdateBatch::Scope updates(batch);
					sidebar->setVisible(showFavorites);
					sidebarRule->setVisible(showFavorites);
					if(!batch.commit())
					{
						klog("navigator: failed to toggle favorites");
						throw std::runtime_error("Browser UI communication failed");
					}
					break;
				}
				case Action::Open:
					if(item.text.empty() || item.text == selectedPath)
						browserOpenSelection(false);
					break;
				case Action::Edit:
					browserOpenSelection(true);
					break;
				case Action::Terminal:
					browserSpawn("/applications/terminal.bin");
					break;
				case Action::NewWindow:
					navigatorOpenWindow(currentPath);
					break;
				case Action::OpenFolderWindow:
					if(item.text == selectedPath && browserSelection() && browserSelection()->directory)
						navigatorOpenWindow(item.text);
					break;
				case Action::Properties:
					browserShowProperties();
					break;
				case Action::Close:
					browserClose();
					break;
				case Action::Location:
					browserFocus(location);
					location->setNumericProperty(ComponentProperty::SelectAll, true);
					break;
				case Action::Find:
					browserFocus(filterBox);
					filterBox->setNumericProperty(ComponentProperty::SelectAll, true);
					break;
				case Action::Select:
					for(auto& tile: tiles)
						if(tile->active && tile->path == item.text)
						{
							browserSelect(item.text);
							break;
						}
					break;
				case Action::Deselect:
					browserSelect("");
					browserFocus(content);
					break;
				case Action::MoveSelection:
				{
					if(visible.empty())
						break;
					int index = -1;
					for(size_t i = 0; i < visible.size(); ++i)
						if(navigator::resolvePath(currentPath, visible[i].name) == selectedPath)
							index = i;
					int step = item.value == 75 ? -1 : item.value == 77 ? 1 : item.value == 72 ? -columns : columns;
					index = index < 0 ? 0 : std::max(0, std::min(static_cast<int>(visible.size()) - 1, index + step));
					browserSelect(navigator::resolvePath(currentPath, visible[index].name));
					uint32_t offset = 0;
					scroller->getNumericProperty(ComponentProperty::ScrollOffsetY, &offset);
					auto bounds = tiles[index]->panel->getBounds();
					int top = bounds.y;
					int bottom = top + bounds.height, height = scroller->getBounds().height;
					if(top < static_cast<int>(offset))
						scroller->setNumericProperty(ComponentProperty::ScrollOffsetY, top);
					else if(bottom > static_cast<int>(offset) + height)
						scroller->setNumericProperty(ComponentProperty::ScrollOffsetY, std::max(0, bottom - height));
					break;
				}
				case Action::Dismiss:
					location->setTitle(currentPath);
					if(item.value == 0)
						browserFocus(content);
					break;
				default:
					break;
			}
		}
	};
} // namespace

namespace
{
	std::vector<std::shared_ptr<BrowserWindow>> browsers;
	void navigatorReapWindows()
	{
		browsers.erase(
				std::remove_if(browsers.begin(), browsers.end(), [](const auto& browser) { return browser->closed; }),
				browsers.end());
	}
} // namespace

void navigatorOpenWindow(const std::string& path)
{
	navigatorEnqueue(
			[path]
			{
				auto browser = std::make_shared<BrowserWindow>();
				try
				{
					browser->browserOpen(path);
					browsers.push_back(browser);
				}
				catch(const std::exception& error)
				{
					klog("navigator: %s", error.what());
					browser->browserClose();
				}
			});
}
