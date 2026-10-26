#ifndef GHOST_EDITOR_NOTEPAD_HPP
#define GHOST_EDITOR_NOTEPAD_HPP

#include "document.hpp"
#include "file_io.hpp"
#include <functional>
#include <cstdlib>

namespace editor
{
enum class Command
{
	None, New, Open, Save, SaveAs, Exit, Undo, Redo, Cut, Copy, Paste, Delete,
	Find, FindNext, Replace, GoTo, SelectAll, Wrap, Font, Status, About,
	Accept, Cancel, Discard, ConfirmSave, ConfirmOverwrite, ReplaceOne, ReplaceAll,
	MatchCase, BrowseUp, Smaller, Larger
};
enum class Dialog { None, Open, Save, Unsaved, Overwrite, Find, Replace, GoTo, Font, About, Error };
struct Key
{
	int scan = 0;
	bool ctrl = false, alt = false, shift = false;
	std::string text = "";
};
enum class Mouse { Move, Press, Release, Drag, Wheel, Right };
struct Rect
{
	int x = 0, y = 0, width = 0, height = 0;
	bool contains(int px, int py) const { return px >= x && py >= y && px < x + width && py < y + height; }
};
struct Entry { std::string name; bool directory; };
struct Item { const char* title; const char* shortcut; Command command; bool enabled = true; bool checked = false; };
struct Control { Rect rect; Command command; int field = -1; };

class Notepad
{
	Command pending = Command::None;
	int goalColumn = -1;
	bool selecting = false;
	int fieldSelecting = -1;
	int scrollDrag = 0, scrollOffset = 0;
	std::string overwritePath;

	void afterMove(bool vertical = false)
	{
		if(!vertical) goalColumn = -1;
		view.layout(document); view.reveal(document);
	}
	void runPending()
	{
		auto command = pending; pending = Command::None;
		dialog = Dialog::None;
		if(command == Command::Exit) quit = true;
		else if(command == Command::New) { document.reset(); path.clear(); format = {}; view.top = view.left = 0; }
		else if(command == Command::Open) showFile(false);
	}
	void protect(Command command)
	{
		pending = command; document.endGroup();
		if(document.dirty()) { dialog = Dialog::Unsaved; message.clear(); dialogFocus = 0; }
		else runPending();
	}
	bool saveTo(const std::string& target)
	{
		// Ghost's create-open can create parent directories. A file selector
		// should require an existing destination instead of creating typo paths.
		if(isDirectory && !isDirectory(parentPath(target)))
		{
			message = "The destination folder does not exist: " + parentPath(target);
			return false;
		}
		if(!writeFile(target, document.text, format, message)) return false;
		path = target; directory = parentPath(path); document.markSaved();
		dialog = Dialog::None;
		if(pending != Command::None) runPending();
		return true;
	}
	void populateDirectory()
	{
		entries = listDirectory ? listDirectory(directory) : std::vector<Entry>{};
		std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b)
		{
			if(a.directory != b.directory) return a.directory;
			return fold(a.name) < fold(b.name);
		});
		fileTop = 0; fileSelected = -1;
	}
	void showFile(bool save)
	{
		dialog = save ? Dialog::Save : Dialog::Open; message.clear(); dialogFocus = 0;
		if(!path.empty()) directory = parentPath(path);
		fields[0].reset(save ? (path.empty() ? resolvePath(directory, "Untitled.txt") : path) : directory + "/");
		fields[0].selectAll(); populateDirectory();
	}
	void search(bool backwards = false)
	{
		if(searchText.empty()) { command(Command::Find); return; }
		if(!document.find(searchText, matchCase, backwards))
		{
			message = "Cannot find \"" + searchText + "\".";
			if(dialog == Dialog::None) { dialog = Dialog::Error; dialogFocus = 0; }
		}
		else { message.clear(); afterMove(); }
	}
	void acceptFile()
	{
		if(fields[0].text.empty()) { message = "Enter a file name or path."; return; }
		auto target = resolvePath(directory, fields[0].text);
		if(isDirectory && isDirectory(target))
		{
			directory = target; populateDirectory(); fields[0].reset(directory == "/" ? "/" : directory + "/");
			fields[0].move(fields[0].text.size()); message.clear(); dialogFocus = 0; return;
		}
		if(dialog == Dialog::Open) load(target);
		else if(target != path && exists && exists(target))
		{
			overwritePath = target; dialog = Dialog::Overwrite; dialogFocus = 1; message.clear();
		}
		else saveTo(target);
	}
public:
	Document document, fields[2];
	TextView view;
	FileFormat format;
	std::string path, directory = "/user", clipboard, message, searchText, replacement;
	Dialog dialog = Dialog::None;
	bool quit = false, showStatus = true, focused = true, matchCase = false;
	bool nativeChrome = false;
	bool caretVisible = true;
	uint64_t caretResetAt = 0;
	int width = 760, height = 500, cell = 9, line = 20, fontSize = 14;
	int menu = -1, menuSelection = -1, dialogFocus = 0, fileTop = 0, fileSelected = -1;
	int mouseX = 0, mouseY = 0;
	int fieldLeft[2] = {0, 0};
	bool shiftDown = false;
	Rect menuRect, dialogRect, verticalTrack, verticalThumb, horizontalTrack, horizontalThumb, fileListRect;
	std::vector<Control> controls;
	std::function<std::vector<Entry>(const std::string&)> listDirectory;
	std::function<bool(const std::string&)> exists, isDirectory;
	std::vector<Entry> entries;
	void resetCaret(uint64_t now) { caretVisible = true; caretResetAt = now; }
	bool caretActive() const { return focused && menu < 0 && (dialog == Dialog::None || (!nativeChrome && dialogFocus < fieldCount())); }
	bool updateCaret(uint64_t now)
	{
		bool visible = caretActive() && ((now - caretResetAt) / 500) % 2 == 0;
		bool changed = caretVisible != visible; caretVisible = visible; return changed;
	}
	void setFocused(bool value, uint64_t now)
	{
		focused = value;
		if(value) resetCaret(now); else caretVisible = false;
	}
	const char* pointerCursor() const
	{
		if(dialog != Dialog::None)
		{
			for(const auto& control : controls)
				if(control.field >= 0 && control.rect.contains(mouseX, mouseY)) return "text";
			return "default";
		}
		return menu < 0 && textRect().contains(mouseX, mouseY) ? "text" : "default";
	}

	Rect textRect() const {
		if(nativeChrome) return {10, 8, std::max(1, width - 28), std::max(1, height - 22)};
		return {4, 29, std::max(1, width - 24), std::max(1, height - 29 - 18 - (showStatus ? 24 : 0))};
	}
	bool browseDirectory(const std::string& value)
	{
		auto target = resolvePath(directory, value);
		if(isDirectory && !isDirectory(target)) { message = "Folder not found: " + target; return false; }
		auto name = dialog == Dialog::Save ? baseName(fields[0].text) : "";
		directory = target; populateDirectory();
		fields[0].reset(name.empty() ? "" : name); message.clear(); return true;
	}
	void resize(int w, int h)
	{
		width = w; height = h;
		auto area = textRect();
		view.width = std::max(1, area.width / cell);
		view.height = std::max(1, area.height / line);
		view.layout(document);
	}
	bool load(const std::string& target)
	{
		std::string text; FileFormat loadedFormat;
		if(!readFile(target, text, loadedFormat, message)) return false;
		document.reset(std::move(text)); path = target; format = loadedFormat;
		directory = parentPath(path); view.top = view.left = 0;
		dialog = Dialog::None; message.clear(); goalColumn = -1; view.layout(document); return true;
	}
	std::vector<Item> menuItems(int index) const
	{
		switch(index)
		{
			case 0: return {{"New", "Ctrl+N", Command::New}, {"Open...", "Ctrl+O", Command::Open},
				{"Save", "Ctrl+S", Command::Save}, {"Save As...", "Ctrl+Shift+S", Command::SaveAs},
				{"", "", Command::None}, {"Exit", "Alt+F4", Command::Exit}};
			case 1: return {{"Undo", "Ctrl+Z", Command::Undo, document.canUndo()},
				{"Redo", "Ctrl+Y", Command::Redo, document.canRedo()}, {"", "", Command::None},
				{"Cut", "Ctrl+X", Command::Cut, document.selected()}, {"Copy", "Ctrl+C", Command::Copy, document.selected()},
				{"Paste", "Ctrl+V", Command::Paste, !clipboard.empty()}, {"Delete", "Del", Command::Delete, document.selected()},
				{"", "", Command::None}, {"Find...", "Ctrl+F", Command::Find},
				{"Find Next", "F3", Command::FindNext}, {"Replace...", "Ctrl+H", Command::Replace},
				{"Go To...", "Ctrl+G", Command::GoTo}, {"", "", Command::None},
				{"Select All", "Ctrl+A", Command::SelectAll}};
			case 2: return {{"Word Wrap", "", Command::Wrap, true, view.wrap}, {"Font...", "", Command::Font}};
			case 3: return {{"Status Bar", "", Command::Status, true, showStatus}};
			default: return {{"About Editor", "", Command::About}};
		}
	}
	void openMenu(int index)
	{
		if(dialog != Dialog::None) return;
		menu = index; menuSelection = -1; document.endGroup(); selecting = false;
	}
	void command(Command action)
	{
		menu = -1; document.endGroup(); selecting = false;
		switch(action)
		{
			case Command::New: case Command::Open: case Command::Exit:
				if(dialog == Dialog::None) protect(action);
				break;
			case Command::Save:
				if(path.empty()) showFile(true);
				else if(!saveTo(path)) { dialog = Dialog::Error; dialogFocus = 0; }
				break;
			case Command::SaveAs: showFile(true); break;
			case Command::Undo: document.undo(); afterMove(); break;
			case Command::Redo: document.redo(); afterMove(); break;
			case Command::Cut:
				if(document.selected()) { clipboard = document.selection(); document.erase(true); afterMove(); }
				break;
			case Command::Copy: if(document.selected()) clipboard = document.selection(); break;
			case Command::Paste: if(!clipboard.empty()) { document.insert(clipboard); afterMove(); } break;
			case Command::Delete: document.erase(false); afterMove(); break;
			case Command::SelectAll: document.selectAll(); afterMove(); break;
			case Command::Wrap: view.wrap = !view.wrap; resize(width, height); afterMove(); break;
			case Command::Status: showStatus = !showStatus; resize(width, height); afterMove(); break;
			case Command::Find: case Command::Replace:
				dialog = action == Command::Find ? Dialog::Find : Dialog::Replace;
				if(document.selected() && document.selection().find('\n') == std::string::npos) searchText = document.selection();
				fields[0].reset(searchText); fields[0].selectAll(); fields[1].reset(replacement);
				dialogFocus = 0; message.clear(); break;
			case Command::FindNext: search(); break;
			case Command::GoTo:
				dialog = Dialog::GoTo; fields[0].reset(std::to_string(document.lineNumber())); fields[0].selectAll();
				dialogFocus = 0; message.clear(); break;
			case Command::Font: dialog = Dialog::Font; dialogFocus = 0; message.clear(); break;
			case Command::About: dialog = Dialog::About; dialogFocus = 0; message.clear(); break;
			case Command::Smaller: fontSize = std::max(10, fontSize - 2); break;
			case Command::Larger: fontSize = std::min(28, fontSize + 2); break;
			case Command::Cancel:
				if(dialog == Dialog::Overwrite) { dialog = Dialog::Save; dialogFocus = 0; }
				else { dialog = Dialog::None; pending = Command::None; message.clear(); }
				break;
			case Command::Discard: runPending(); break;
			case Command::ConfirmSave:
				if(path.empty()) showFile(true);
				else if(!saveTo(path)) { dialog = Dialog::Unsaved; dialogFocus = 0; }
				break;
			case Command::ConfirmOverwrite:
				dialog = Dialog::Save; dialogFocus = 0; saveTo(overwritePath); break;
			case Command::MatchCase: matchCase = !matchCase; break;
			case Command::BrowseUp:
				directory = parentPath(directory); populateDirectory(); fields[0].reset(directory == "/" ? "/" : directory + "/");
				fields[0].move(fields[0].text.size()); dialogFocus = 0; message.clear(); break;
			case Command::ReplaceOne: case Command::ReplaceAll:
				searchText = fields[0].text; replacement = fields[1].text;
				if(searchText.empty()) { message = "Enter the text to find."; break; }
				if(action == Command::ReplaceAll)
				{
					auto count = document.replaceAll(searchText, replacement, matchCase);
					message = std::to_string(count) + " occurrence(s) replaced."; afterMove();
				}
				else
				{
					if(document.selected() && (matchCase ? document.selection() == searchText : fold(document.selection()) == fold(searchText)))
						document.insert(replacement);
					search();
				}
				break;
			case Command::Accept:
				if(dialog == Dialog::Open || dialog == Dialog::Save) acceptFile();
				else if(dialog == Dialog::Find || dialog == Dialog::Replace)
				{
					searchText = fields[0].text;
					if(searchText.empty()) message = "Enter the text to find.";
					else search();
				}
				else if(dialog == Dialog::GoTo)
				{
					auto& value = fields[0].text;
					bool numeric = !value.empty() && value.size() < 10 && value.find_first_not_of("0123456789") == std::string::npos;
					if(!numeric || !document.goTo(strtoul(value.c_str(), nullptr, 10))) message = "Enter an existing line number.";
					else { dialog = Dialog::None; afterMove(); }
				}
				else { dialog = Dialog::None; pending = Command::None; message.clear(); }
				break;
			default: break;
		}
	}
	std::vector<Command> dialogCommands() const
	{
		switch(dialog)
		{
			case Dialog::Unsaved: return {Command::ConfirmSave, Command::Discard, Command::Cancel};
			case Dialog::Overwrite: return {Command::ConfirmOverwrite, Command::Cancel};
			case Dialog::Find: return {Command::MatchCase, Command::Accept, Command::Cancel};
			case Dialog::Replace: return {Command::MatchCase, Command::Accept, Command::ReplaceOne, Command::ReplaceAll, Command::Cancel};
			case Dialog::Open: case Dialog::Save: return {Command::Accept, Command::Cancel, Command::BrowseUp};
			case Dialog::GoTo: return {Command::Accept, Command::Cancel};
			case Dialog::Font: return {Command::Smaller, Command::Larger, Command::Accept};
			default: return {Command::Accept};
		}
	}
	int fieldCount() const
	{
		return dialog == Dialog::Replace ? 2 : dialog == Dialog::Open || dialog == Dialog::Save || dialog == Dialog::Find || dialog == Dialog::GoTo ? 1 : 0;
	}
	void key(const Key& key)
	{
		shiftDown = key.shift;
		if(key.scan == 29 || key.scan == 42 || key.scan == 54 || key.scan == 56 || key.scan == 58) return;
		if(dialog != Dialog::None)
		{
			int fieldsCount = fieldCount();
			auto buttons = dialogCommands();
			if((dialog == Dialog::Open || dialog == Dialog::Save) && key.alt && key.scan == 72)
			{ command(Command::BrowseUp); return; }
			if((dialog == Dialog::Open || dialog == Dialog::Save) && (key.scan == 72 || key.scan == 80))
			{
				if(entries.empty()) return;
				fileSelected = fileSelected < 0 ? 0 : std::max(0, std::min(static_cast<int>(entries.size()) - 1, fileSelected + (key.scan == 72 ? -1 : 1)));
				fields[0].reset(resolvePath(directory, entries[fileSelected].name)); fields[0].move(fields[0].text.size());
				int visible = std::max(1, fileListRect.height / 22);
				fileTop = std::max(0, std::min(fileTop, fileSelected));
				if(fileSelected >= fileTop + visible) fileTop = fileSelected - visible + 1;
				dialogFocus = 0; return;
			}
			if(key.scan == 1) { command(Command::Cancel); return; }
			if(key.scan == 15)
			{
				int count = fieldsCount + buttons.size();
				dialogFocus = (dialogFocus + count + (key.shift ? -1 : 1)) % count; return;
			}
			if(key.scan == 28 || (key.text == " " && dialogFocus >= fieldsCount))
			{
				command(dialogFocus >= fieldsCount ? buttons[dialogFocus - fieldsCount] : Command::Accept); return;
			}
			if(dialogFocus >= fieldsCount) return;
			auto& field = fields[dialogFocus];
			if(key.ctrl && key.scan == 30) field.selectAll();
			else if(key.ctrl && (key.scan == 46 || key.scan == 45))
			{
				if(field.selected()) { clipboard = field.selection(); if(key.scan == 45) field.erase(true); }
			}
			else if(key.ctrl && key.scan == 47)
			{
				auto value = clipboard; value.erase(std::remove(value.begin(), value.end(), '\n'), value.end()); field.insert(value);
			}
			else if(key.ctrl && key.scan == 44) { if(key.shift) field.redo(); else field.undo(); }
			else if(key.ctrl && key.scan == 21) field.redo();
			else if(key.scan == 75) field.horizontal(-1, key.ctrl, key.shift);
			else if(key.scan == 77) field.horizontal(1, key.ctrl, key.shift);
			else if(key.scan == 71) field.move(0, key.shift);
			else if(key.scan == 79) field.move(field.text.size(), key.shift);
			else if(key.scan == 14 || key.scan == 83) field.erase(key.scan == 14, key.ctrl);
			else if(!key.text.empty() && !key.ctrl && !key.alt && key.text != "\n" && key.text != "\t") field.insert(key.text, true);
			return;
		}
		if(key.alt)
		{
			if(key.scan == 62) { command(Command::Exit); return; }
			int index = key.scan == 33 ? 0 : key.scan == 18 ? 1 : key.scan == 24 ? 2 : key.scan == 47 ? 3 : key.scan == 35 ? 4 : -1;
			if(index >= 0) { openMenu(index); return; }
		}
		if(key.scan == 68) { if(menu < 0) openMenu(0); else menu = -1; return; } // F10
		if(menu >= 0)
		{
			if(key.scan == 1) menu = -1;
			else if(key.scan == 75 || key.scan == 77) openMenu((menu + (key.scan == 75 ? 4 : 1)) % 5);
			else if(key.scan == 72 || key.scan == 80)
			{
				auto items = menuItems(menu); int count = items.size();
				for(int i = 0; i < count; ++i)
				{
					menuSelection = menuSelection < 0 ? (key.scan == 72 ? count - 1 : 0) :
						(menuSelection + count + (key.scan == 72 ? -1 : 1)) % count;
					if(items[menuSelection].enabled && items[menuSelection].command != Command::None) break;
				}
			}
			else if(key.scan == 28)
			{
				auto items = menuItems(menu);
				if(menuSelection >= 0 && items[menuSelection].enabled) command(items[menuSelection].command);
			}
			return;
		}
		if(key.ctrl)
		{
			Command action = key.scan == 49 ? Command::New : key.scan == 24 ? Command::Open :
				key.scan == 31 ? (key.shift ? Command::SaveAs : Command::Save) :
				key.scan == 44 ? (key.shift ? Command::Redo : Command::Undo) : key.scan == 21 ? Command::Redo :
				key.scan == 45 ? Command::Cut : key.scan == 46 ? Command::Copy : key.scan == 47 ? Command::Paste :
				key.scan == 30 ? Command::SelectAll : key.scan == 33 ? Command::Find : key.scan == 35 ? Command::Replace :
				key.scan == 34 ? Command::GoTo : key.scan == 17 ? Command::Exit : Command::None;
			if(action != Command::None) { command(action); return; }
			if(key.scan == 82) { command(Command::Copy); return; }
		}
		if(key.shift && key.scan == 82) { command(Command::Paste); return; }
		if(key.shift && key.scan == 83) { command(Command::Cut); return; }
		if(key.scan == 61) { search(key.shift); return; }
		if(key.scan == 59) { command(Command::About); return; }
		view.layout(document);
		if(key.scan == 75 || key.scan == 77) document.horizontal(key.scan == 75 ? -1 : 1, key.ctrl, key.shift);
		else if(key.scan == 71 || key.scan == 79)
		{
			auto row = view.rows[view.rowAt(document.cursor)];
			document.move(key.ctrl ? (key.scan == 71 ? 0 : document.text.size()) :
				(key.scan == 71 ? row.start : row.end), key.shift);
		}
		else if(key.scan == 72 || key.scan == 80 || key.scan == 73 || key.scan == 81)
		{
			if(key.ctrl && (key.scan == 72 || key.scan == 80))
			{
				view.top += key.scan == 72 ? -1 : 1; view.clamp(); return;
			}
			if(goalColumn < 0) goalColumn = view.columnAt(document, document.cursor);
			int delta = key.scan == 72 ? -1 : key.scan == 80 ? 1 : key.scan == 73 ? -view.height : view.height;
			document.move(view.position(document, view.rowAt(document.cursor) + delta, goalColumn), key.shift);
			afterMove(true); return;
		}
		else if(key.scan == 14 || key.scan == 83) document.erase(key.scan == 14, key.ctrl);
		else if(!key.ctrl && !key.alt)
		{
			if(key.scan == 28) document.insert("\n");
			else if(key.scan == 15) document.insert("\t");
			else if(!key.text.empty()) document.insert(key.text, true);
		}
		afterMove();
	}
	void mouse(Mouse kind, int x, int y, int value = 0)
	{
		mouseX = x; mouseY = y;
		if(kind == Mouse::Release) { selecting = false; fieldSelecting = -1; scrollDrag = 0; return; }
		if(dialog != Dialog::None)
		{
			if(kind == Mouse::Drag && fieldSelecting >= 0)
			{
				for(const auto& control : controls) if(control.field == fieldSelecting)
				{
					auto& field = fields[fieldSelecting];
					int col = std::max(0, (x - control.rect.x - 6) / cell + fieldLeft[fieldSelecting]);
					size_t p = 0; while(col-- > 0 && p < field.text.size()) p = next(field.text, p);
					field.move(p, true); return;
				}
			}
			if(kind == Mouse::Wheel && fileListRect.contains(x, y))
			{
				fileTop = std::max(0, std::min(fileTop + value * 3, std::max(0, static_cast<int>(entries.size()) - fileListRect.height / 22))); return;
			}
			if(kind != Mouse::Press) return;
			for(const auto& control : controls) if(control.rect.contains(x, y))
			{
				if(control.field >= 0)
				{
					dialogFocus = control.field;
					auto& field = fields[dialogFocus];
					int col = std::max(0, (x - control.rect.x - 6) / cell);
					col += fieldLeft[dialogFocus];
					size_t p = 0; while(col-- > 0 && p < field.text.size()) p = next(field.text, p);
					field.move(p, shiftDown);
					if(value == 2) field.selectAll();
					fieldSelecting = dialogFocus;
				}
				else command(control.command);
				return;
			}
			if((dialog == Dialog::Open || dialog == Dialog::Save) && fileListRect.contains(x, y))
			{
				int index = fileTop + (y - fileListRect.y) / 22;
				if(index >= static_cast<int>(entries.size())) return;
				fileSelected = index;
				fields[0].reset(resolvePath(directory, entries[index].name)); fields[0].move(fields[0].text.size());
				dialogFocus = 0;
				if(value == 2) acceptFile();
			}
			return;
		}
		if(kind == Mouse::Right) { openMenu(1); return; }
		if(!nativeChrome && y < 26 && (kind == Mouse::Press || (kind == Mouse::Move && menu >= 0)))
		{
			int edges[] = {0, 46, 92, 158, 210, 260};
			for(int i = 0; i < 5; ++i) if(x >= edges[i] && x < edges[i + 1])
			{
				if(kind == Mouse::Press && menu == i) menu = -1; else if(menu != i) openMenu(i);
				return;
			}
		}
		if(menu >= 0)
		{
			if(menuRect.contains(x, y))
			{
				auto items = menuItems(menu); int index = (y - menuRect.y - 3) / 24;
				if(index >= 0 && index < static_cast<int>(items.size()))
				{
					if(kind == Mouse::Move) menuSelection = index;
					if(kind == Mouse::Press && items[index].enabled && items[index].command != Command::None) command(items[index].command);
				}
				return;
			}
			if(kind == Mouse::Press) menu = -1; else return;
		}
		view.layout(document);
		if(kind == Mouse::Wheel)
		{
			if(shiftDown) view.left += value * 3; else view.top += value * 3;
			view.clamp(); return;
		}
		if(kind == Mouse::Press && (verticalTrack.contains(x, y) || horizontalTrack.contains(x, y)))
		{
			bool vertical = verticalTrack.contains(x, y);
			auto thumb = vertical ? verticalThumb : horizontalThumb;
			int coordinate = vertical ? y : x, origin = vertical ? thumb.y : thumb.x;
			if(thumb.contains(x, y)) { scrollDrag = vertical ? 1 : 2; scrollOffset = coordinate - origin; }
			else
			{
				int step = coordinate < origin ? -1 : 1;
				if(vertical) view.top += step * view.height; else view.left += step * view.width;
				view.clamp();
			}
			return;
		}
		if(kind == Mouse::Drag && scrollDrag)
		{
			if(scrollDrag == 1)
				view.top = static_cast<int64_t>(y - scrollOffset - verticalTrack.y) * std::max(0, static_cast<int>(view.rows.size()) - view.height) /
					std::max(1, verticalTrack.height - verticalThumb.height);
			else view.left = static_cast<int64_t>(x - scrollOffset - horizontalTrack.x) * std::max(0, view.maxColumns + 1 - view.width) /
				std::max(1, horizontalTrack.width - horizontalThumb.width);
			view.clamp(); return;
		}
		auto area = textRect();
		if((kind == Mouse::Press && area.contains(x, y)) || (kind == Mouse::Drag && selecting))
		{
			if(kind == Mouse::Drag)
			{
				if(y < area.y) --view.top; else if(y >= area.y + area.height) ++view.top;
				if(x < area.x) --view.left; else if(x >= area.x + area.width) ++view.left;
				view.clamp();
			}
			int row = std::max(0, (y - area.y) / line) + view.top;
			int col = std::max(0, (x - area.x + cell / 2) / cell) + view.left;
			auto p = view.position(document, row, col);
			document.move(p, kind == Mouse::Drag || shiftDown); selecting = true; goalColumn = -1;
			if(kind == Mouse::Press && value == 2 && p < document.text.size())
			{
				int type = Document::wordKind(document.text[p]); auto start = p, end = p;
				while(start && Document::wordKind(document.text[previous(document.text, start)]) == type) start = previous(document.text, start);
				while(end < document.text.size() && Document::wordKind(document.text[end]) == type) end = next(document.text, end);
				document.anchor = start; document.cursor = end;
			}
			else if(kind == Mouse::Press && value >= 3)
			{
				document.anchor = document.lineStart(p); document.cursor = document.lineEnd(p);
				if(document.cursor < document.text.size()) ++document.cursor;
			}
		}
	}
};
}
#endif
