// Ghost Editor — classic Notepad style plain text editing.
// Copyright (C) 2026 Max Schlüssel. GPL-3.0-or-later.

#include "notepad.hpp"
#include "render.hpp"
#include "ui.hpp"
#include <libfenster/application.hpp>
#include <libfenster/components/canvas.hpp>
#include <libfenster/components/window.hpp>
#include <libfenster/font/font_loader.hpp>
#include <libfenster/layout/grid_layout.hpp>
#include <deque>
#include <sys/stat.h>

using namespace fenster;

namespace
{
enum class EventType { Key, Mouse, Resize, Repaint, Focus, Close, Ui };
struct Event
{
	EventType type;
	KeyInfoBasic key;
	ComponentMouseEvent mouse{};
	Rectangle bounds;
	bool focused = false;
	editor::UiEvent ui;
};
std::deque<Event> events;
g_user_mutex eventLock = g_mutex_initialize();
g_user_mutex eventReady = g_mutex_initialize();

void post(Event event)
{
	g_mutex_acquire(eventLock);
	if(!events.empty() && event.type == events.back().type &&
		(event.type == EventType::Resize || event.type == EventType::Repaint)) events.back() = event;
	else events.push_back(event);
	g_mutex_release(eventLock); g_mutex_release(eventReady);
}
bool isDirectory(const std::string& path)
{
	struct stat info{};
	return stat(path.c_str(), &info) == 0 && S_ISDIR(info.st_mode);
}
std::vector<editor::Entry> listDirectory(const std::string& path)
{
	std::vector<editor::Entry> entries;
	auto directory = g_open_directory(path.c_str());
	if(!directory) return entries;
	while(auto entry = g_read_directory(directory))
	{
		if(std::string(entry->name) == "." || std::string(entry->name) == "..") continue;
		bool folder = entry->type == G_FS_NODE_TYPE_FOLDER || entry->type == G_FS_NODE_TYPE_ROOT || entry->type == G_FS_NODE_TYPE_MOUNTPOINT;
		if(folder || entry->type == G_FS_NODE_TYPE_FILE) entries.push_back({entry->name, folder});
	}
	g_close_directory(directory); return entries;
}
editor::Key translate(const KeyInfoBasic& key)
{
	g_key_info_basic basic;
	basic.pressed = key.pressed; basic.ctrl = key.ctrl; basic.alt = key.alt;
	basic.shift = key.shift; basic.scancode = key.scancode;
	auto full = g_keyboard::fullKeyInfo(basic);
	char c = g_keyboard::charForKey(full);
	std::string text;
	if(c != static_cast<char>(-1) && static_cast<unsigned char>(c) >= 32)
		editor::appendUtf8(text, static_cast<unsigned char>(c));
	int scan = key.scancode & 0xff;
	if(key.ctrl && full.key == "KEY_Z") scan = 44;
	else if(key.ctrl && full.key == "KEY_Y") scan = 21;
	// AltGr characters come from the layout's Ctrl+Alt conversion entries.
	if(key.ctrl && key.alt && !text.empty()) return {scan, false, false, key.shift, text};
	return {scan, key.ctrl, key.alt, key.shift, text};
}
}

int main(int argc, char* argv[])
{
	g_mutex_acquire(eventReady);
	if(Application::open() != ApplicationOpenStatus::Success) { klog("editor: failed to create UI"); return -1; }
	if(!g_keyboard::loadLayout("de-DE")) { klog("editor: failed to load keyboard layout"); return -1; }
	auto window = Window::create();
	if(!window) { klog("editor: failed to create window"); return -1; }
	window->setBounds(Rectangle(80, 70, 780, 560)); window->setMinimumSize(Dimension(560, 440));
	editor::Notepad app;
	app.listDirectory = listDirectory; app.isDirectory = isDirectory;
	app.exists = [](const std::string& path) { struct stat info{}; return stat(path.c_str(), &info) == 0; };
	char workingDirectory[G_PATH_MAX];
	if(g_get_working_directory(workingDirectory) == G_GET_WORKING_DIRECTORY_SUCCESSFUL) app.directory = workingDirectory;
	if(argc > 1 && !app.load(editor::resolvePath(app.directory, argv[1]))) app.dialog = editor::Dialog::Error;
    app.nativeChrome = true;
    editor::EditorUi ui(app, window, [](editor::UiEvent value) { Event event{EventType::Ui}; event.ui = std::move(value); post(std::move(event)); });
    auto canvas = ui.canvas;
    if(!canvas) { klog("editor: failed to create document canvas"); return -1; }
    canvas->setBoundsListener([](Rectangle bounds) { Event event{EventType::Resize}; event.bounds = bounds; post(event); });
    canvas->setBufferListener([]() { post({EventType::Repaint}); });
    canvas->addKeyListener([](KeyEvent& key) { Event event{EventType::Key}; event.key = key.info; post(event); });
    canvas->addMouseListener([](ComponentMouseEvent* mouse) { Event event{EventType::Mouse}; event.mouse = *mouse; post(event); });
    window->onClose([]() { post({EventType::Close}); });
    window->addFocusListener([](bool focused) { Event event{EventType::Focus}; event.focused = focused; post(event); });
    window->addKeyListener([](KeyEvent& key) { Event event{EventType::Key}; event.key = key.info; post(event); });

	auto uiFont = FontLoader::getDefault(); auto monoFont = FontLoader::get("consolas");
	if(!uiFont || !monoFont) { klog("editor: failed to load fonts"); return -1; }
	window->setTitle("Untitled - Editor"); window->setVisible(true); canvas->setFocused(true);
	std::string lastTitle;
	std::string lastPointer;
	bool needsPaint = true;
	app.resetCaret(g_millis());
	auto updatePointer = [&]()
	{
		std::string shape = app.pointerCursor();
		if(shape != lastPointer) { canvas->setStringProperty(ComponentProperty::MouseCursor, shape); lastPointer = shape; }
	};
	editor::Painter painter(nullptr, uiFont->getFace(), monoFont->getFace());
	auto paint = [&](bool caretOnly = false)
	{
		if(!caretOnly)
		{
			ui.sync();
			auto title = std::string(app.document.dirty() ? "*" : "") + (app.path.empty() ? "Untitled" : editor::baseName(app.path)) + " - Editor";
			if(title != lastTitle) { window->setTitle(title); lastTitle = title; }
		}
		if(auto cr = canvas->acquireGraphics())
		{
			painter.setContext(cr); auto damage = painter.paint(app, caretOnly);
			if(damage.width && damage.height) canvas->blit(Rectangle(damage.x, damage.y, damage.width, damage.height));
			canvas->releaseGraphics();
			needsPaint = false;
		}
		if(!caretOnly) updatePointer();
	};
	for(;;)
	{
		// Wake at the next blink boundary even when no input arrives.
		auto elapsed = g_millis() - app.caretResetAt;
		g_mutex_acquire_to(eventReady, app.caretActive() ? 500 - elapsed % 500 : 9999);
		for(;;)
		{
			g_mutex_acquire(eventLock);
			if(events.empty()) { g_mutex_release(eventLock); break; }
			auto event = std::move(events.front()); events.pop_front(); g_mutex_release(eventLock);
			auto oldDialog = app.dialog; int oldMenu = app.menu, oldFont = app.fontSize;
			bool changed = true;
			if(event.type == EventType::Resize) app.resize(event.bounds.width, event.bounds.height);
			else if(event.type == EventType::Key)
			{
				app.shiftDown = event.key.shift;
				if(event.key.pressed && app.dialog == editor::Dialog::None) { app.resetCaret(g_millis()); app.key(translate(event.key)); }
				int scan = event.key.scancode & 0xff;
				changed = event.key.pressed && scan != 29 && scan != 42 && scan != 54 && scan != 56 && scan != 58;
			}
			else if(event.type == EventType::Focus)
            {
                changed = app.focused != event.focused || app.caretVisible != event.focused;
                app.setFocused(event.focused && app.dialog == editor::Dialog::None, g_millis());
                if(event.focused && app.dialog != editor::Dialog::None) ui.focusDialog();
                if(!event.focused) app.menu = -1;
            }
            else if(event.type == EventType::Ui)
            {
                if(event.ui.kind == editor::UiKind::MainKey) {
                    if(app.dialog == editor::Dialog::None) app.key(translate(event.ui.key));
                    else ui.focusDialog();
                } else changed = ui.handle(event.ui);
            }
			else if(event.type == EventType::Close)
			{
				// Fenster hides the window before Close. Restore it for Save/Cancel.
				if(app.dialog == editor::Dialog::None) app.command(editor::Command::Exit);
				if(!app.quit) { window->setVisible(true); if(app.dialog != editor::Dialog::None) ui.focusDialog(); else canvas->setFocused(true); }
			}
			else if(event.type == EventType::Mouse && app.dialog == editor::Dialog::None)
			{
				auto mouse = event.mouse;
				app.mouseX = mouse.position.x; app.mouseY = mouse.position.y;
				if(mouse.type == MouseEventType::Press || mouse.type == MouseEventType::Drag) app.resetCaret(g_millis());
				if(mouse.type == MouseEventType::Press)
				{
					canvas->setFocused(true);
					app.mouse(mouse.buttons & MouseButton::Button2 ? editor::Mouse::Right : editor::Mouse::Press, mouse.position.x, mouse.position.y, mouse.clickCount);
				}
				else if(mouse.type == MouseEventType::Move)
				{
					int selection = app.menuSelection;
					if(app.menu >= 0) app.mouse(editor::Mouse::Move, mouse.position.x, mouse.position.y);
					changed = oldMenu != app.menu || selection != app.menuSelection;
				}
				else if(mouse.type == MouseEventType::Drag) app.mouse(editor::Mouse::Drag, mouse.position.x, mouse.position.y);
				else if(mouse.type == MouseEventType::Release || mouse.type == MouseEventType::DragRelease)
				{ app.mouse(editor::Mouse::Release, mouse.position.x, mouse.position.y); changed = false; }
				else if(mouse.type == MouseEventType::Scroll) app.mouse(editor::Mouse::Wheel, mouse.position.x, mouse.position.y, mouse.scroll);
				else changed = false;
			}
			if(app.quit) g_exit(0);
			updatePointer();
			needsPaint |= changed;
			g_mutex_acquire(eventLock); bool moreEvents = !events.empty(); g_mutex_release(eventLock);
			// Drain bursts before painting; menu/dialog geometry must be ready for the next click.
			if(!needsPaint || (moreEvents && oldDialog == app.dialog && oldMenu == app.menu && oldFont == app.fontSize &&
				event.type != EventType::Resize && event.type != EventType::Repaint)) continue;
			paint();
		}
		bool caretChanged = app.updateCaret(g_millis());
		if(needsPaint || caretChanged) paint(!needsPaint);
	}
}
