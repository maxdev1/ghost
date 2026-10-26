#ifndef GHOST_EDITOR_RENDER_HPP
#define GHOST_EDITOR_RENDER_HPP

#include "notepad.hpp"
#include <cairo/cairo.h>
#include <cmath>
#include <unordered_map>

namespace editor
{
class Painter
{
	cairo_t* cr;
	cairo_font_face_t *uiFont, *textFont;
	int glyphSize = -1;
	std::unordered_map<uint32_t, std::vector<cairo_glyph_t>> glyphCache;
	std::vector<cairo_glyph_t> normalGlyphs, selectedGlyphs;

	void appendGlyphs(const std::string& value, size_t start, size_t end, int x, int y, bool selected)
	{
		size_t p = start; uint32_t code;
		if(!decodeUtf8(value, p, code) || code < 32 || code == 127) return;
		auto found = glyphCache.find(code);
		if(found == glyphCache.end())
		{
			cairo_glyph_t* glyphs = nullptr; int count = 0;
			auto status = cairo_scaled_font_text_to_glyphs(cairo_get_scaled_font(cr), 0, 0,
				value.data() + start, end - start, &glyphs, &count, nullptr, nullptr, nullptr);
			if(status != CAIRO_STATUS_SUCCESS) { cairo_glyph_free(glyphs); return; }
			std::vector<cairo_glyph_t> cached;
			if(count) cached.assign(glyphs, glyphs + count);
			cairo_glyph_free(glyphs);
			found = glyphCache.emplace(code, std::move(cached)).first;
		}
		auto& output = selected ? selectedGlyphs : normalGlyphs;
		for(auto glyph : found->second) { glyph.x += x; glyph.y += y; output.push_back(glyph); }
	}
public:
	Painter(cairo_t* context, cairo_font_face_t* ui, cairo_font_face_t* mono) : cr(context), uiFont(ui), textFont(mono) {}
	void setContext(cairo_t* context) { cr = context; }
	void fill(Rect r, double red, double green, double blue)
	{
		cairo_set_source_rgb(cr, red, green, blue); cairo_rectangle(cr, r.x, r.y, r.width, r.height); cairo_fill(cr);
	}
    void roundFill(Rect r, double shade)
    {
        double radius = std::min(4., std::min(r.width, r.height) / 2.);
        cairo_new_sub_path(cr); cairo_arc(cr, r.x + r.width - radius, r.y + radius, radius, -1.5707963268, 0);
        cairo_arc(cr, r.x + r.width - radius, r.y + r.height - radius, radius, 0, 1.5707963268);
        cairo_arc(cr, r.x + radius, r.y + r.height - radius, radius, 1.5707963268, 3.1415926536);
        cairo_arc(cr, r.x + radius, r.y + radius, radius, 3.1415926536, 4.7123889804); cairo_close_path(cr);
        cairo_set_source_rgb(cr, shade, shade + .015, shade + .04); cairo_fill(cr);
    }

	void border(Rect r, double shade = .65)
	{
		cairo_set_source_rgb(cr, shade, shade, shade); cairo_set_line_width(cr, 1);
		cairo_rectangle(cr, r.x + .5, r.y + .5, std::max(0, r.width - 1), std::max(0, r.height - 1)); cairo_stroke(cr);
	}
	void text(int x, int y, const std::string& value, double shade = .08, int size = 14, bool mono = false)
	{
		// Invalid file names must never poison Cairo's context with malformed UTF-8.
		std::string safe;
		for(size_t p = 0; p < value.size();)
		{
			auto start = p; uint32_t c;
			if(!decodeUtf8(value, p, c)) { safe += '?'; p = start + 1; }
			else if(c >= 32 && c != 127) safe.append(value, start, p - start);
		}
		cairo_set_font_face(cr, mono ? textFont : uiFont); cairo_set_font_size(cr, size);
		cairo_set_source_rgb(cr, shade, shade, shade); cairo_move_to(cr, x, y); cairo_show_text(cr, safe.c_str());
	}
	void clippedText(Rect r, const std::string& value, double shade = .08, int size = 14, bool mono = false)
	{
		cairo_save(cr); cairo_rectangle(cr, r.x, r.y, r.width, r.height); cairo_clip(cr);
		text(r.x, r.y + size + 2, value, shade, size, mono); cairo_restore(cr);
	}
	void wrappedText(Rect r, const std::string& value)
	{
		int y = r.y; std::string line;
		for(size_t p = 0; p < value.size();)
		{
			auto end = value.find_first_of(" \n", p);
			if(end == std::string::npos) end = value.size();
			auto word = value.substr(p, end - p);
			cairo_set_font_face(cr, uiFont); cairo_set_font_size(cr, 14); cairo_text_extents_t extents;
			cairo_text_extents(cr, (line + word).c_str(), &extents);
			if(extents.x_advance > r.width && !line.empty()) { clippedText({r.x, y, r.width, 20}, line); y += 20; line.clear(); }
			line += word;
			if(end < value.size() && value[end] == '\n') { clippedText({r.x, y, r.width, 20}, line); y += 20; line.clear(); }
			else line += ' ';
			p = end + 1;
		}
		if(!line.empty()) clippedText({r.x, y, r.width, 20}, line);
	}
	void button(Notepad& app, Rect r, const std::string& label, Command action, int index)
	{
		fill(r, .95, .95, .95); border(r, .55);
		if(app.dialogFocus == app.fieldCount() + index) border({r.x + 2, r.y + 2, r.width - 4, r.height - 4}, .25);
		cairo_set_font_face(cr, uiFont); cairo_set_font_size(cr, 14); cairo_text_extents_t extents;
		cairo_text_extents(cr, label.c_str(), &extents);
		text(r.x + std::max(4, (r.width - static_cast<int>(extents.x_advance)) / 2), r.y + 20, label);
		app.controls.push_back({r, action, -1});
	}
	void field(Notepad& app, Rect r, int index)
	{
		auto& doc = app.fields[index];
		fill(r, 1, 1, 1); border(r, app.dialogFocus == index ? .3 : .65);
		int columns = std::max(1, (r.width - 16) / app.cell), cursorCol = 0;
		for(size_t p = 0; p < doc.cursor; p = next(doc.text, p)) ++cursorCol;
		int left = std::max(0, cursorCol - columns + 1);
		app.fieldLeft[index] = left;
		cairo_save(cr); cairo_rectangle(cr, r.x + 4, r.y + 2, r.width - 8, r.height - 4); cairo_clip(cr);
		int column = 0;
		for(size_t p = 0; p < doc.text.size(); p = next(doc.text, p), ++column)
		{
			if(column < left || column >= left + columns) continue;
			int x = r.x + 6 + (column - left) * app.cell;
			bool selected = app.dialogFocus == index && p >= doc.first() && p < doc.last();
			if(selected) fill({x, r.y + 4, app.cell, r.height - 8}, .1, .3, .65);
			text(x, r.y + 21, doc.text.substr(p, next(doc.text, p) - p), selected ? 1 : .08, app.fontSize, true);
		}
		if(app.focused && app.caretVisible && app.dialogFocus == index)
			fill({r.x + 6 + (cursorCol - left) * app.cell, r.y + 5, 2, r.height - 10}, .05, .05, .05);
		cairo_restore(cr); app.controls.push_back({r, Command::None, index});
	}
	void document(Notepad& app, bool caretOnly = false)
	{
		auto& doc = app.document; auto& view = app.view; auto area = app.textRect();
		fill({0, app.nativeChrome ? 0 : 26, app.width, app.height - (app.nativeChrome ? 0 : 26)}, 1, 1, 1);
		cairo_save(cr); cairo_rectangle(cr, area.x, area.y, area.width, area.height); cairo_clip(cr);
		cairo_set_font_face(cr, textFont); cairo_set_font_size(cr, app.fontSize);
		if(glyphSize != app.fontSize) { glyphCache.clear(); glyphSize = app.fontSize; }
		normalGlyphs.clear(); selectedGlyphs.clear();
		cairo_set_source_rgb(cr, app.focused ? .1 : .65, app.focused ? .3 : .7, app.focused ? .65 : .8);
		int firstRow = caretOnly ? view.rowAt(doc.cursor) : view.top;
		int lastRow = caretOnly ? firstRow + 1 : view.top + view.height + 1;
		for(int row = firstRow; row < static_cast<int>(view.rows.size()) && row < lastRow; ++row)
		{
			auto line = view.rows[row]; int column = 0, y = area.y + (row - view.top) * app.line;
			for(size_t p = line.start; p < line.end; p = next(doc.text, p))
			{
				int after = advanceColumn(column, doc.text[p]);
				if(after > view.left && column < view.left + view.width + 1)
				{
					int x = area.x + (column - view.left) * app.cell;
					bool selected = p >= doc.first() && p < doc.last();
					if(selected) cairo_rectangle(cr, x, y, (after - column) * app.cell, app.line);
					if(doc.text[p] != '\t') appendGlyphs(doc.text, p, next(doc.text, p), x, y + app.fontSize, selected);
				}
				column = after; if(column > view.left + view.width + 1) break;
			}
			if(line.end < doc.text.size() && doc.text[line.end] == '\n' && line.end >= doc.first() && line.end < doc.last())
				cairo_rectangle(cr, area.x + (line.columns - view.left) * app.cell, y, app.cell, app.line);
		}
		cairo_fill(cr);
		cairo_set_source_rgb(cr, .03, .03, .03);
		if(!normalGlyphs.empty()) cairo_show_glyphs(cr, normalGlyphs.data(), normalGlyphs.size());
		cairo_set_source_rgb(cr, 1, 1, 1);
		if(!selectedGlyphs.empty()) cairo_show_glyphs(cr, selectedGlyphs.data(), selectedGlyphs.size());
		int row = view.rowAt(doc.cursor), col = view.columnAt(doc, doc.cursor);
		if(app.focused && app.caretVisible && app.dialog == Dialog::None && app.menu < 0)
			fill({area.x + (col - view.left) * app.cell, area.y + (row - view.top) * app.line, 2, app.line - 2}, .05, .05, .05);
		cairo_restore(cr);
		app.verticalTrack = {app.width - 18, 26, 18, area.height + 3};
		app.horizontalTrack = {0, area.y + area.height, app.width - 18, 18};
		if(app.nativeChrome) {
			app.verticalTrack = {app.width - 14, 0, 14, app.height - 14};
			app.horizontalTrack = {0, app.height - 14, app.width - 14, 14};
		}
		auto v = app.verticalTrack, h = app.horizontalTrack;
		fill(v, app.nativeChrome ? .985 : .94, app.nativeChrome ? .988 : .94, app.nativeChrome ? .995 : .94); fill(h, app.nativeChrome ? .985 : .94, app.nativeChrome ? .988 : .94, app.nativeChrome ? .995 : .94);
		int total = std::max(view.height, static_cast<int>(view.rows.size()));
		int thumbHeight = std::min(v.height, std::max(22, v.height * view.height / total));
		app.verticalThumb = {v.x + 3, v.y + static_cast<int>(static_cast<int64_t>(v.height - thumbHeight) * view.top / std::max(1, total - view.height)), v.width - 6, thumbHeight};
		if(app.nativeChrome) { if(total > view.height) roundFill(app.verticalThumb, .78); } else fill(app.verticalThumb, .72, .72, .72);
		total = std::max(view.width, view.maxColumns + 1);
		int thumbWidth = app.view.wrap ? h.width : std::min(h.width, std::max(22, h.width * view.width / total));
		app.horizontalThumb = {h.x + static_cast<int>(static_cast<int64_t>(h.width - thumbWidth) * view.left / std::max(1, total - view.width)), h.y + 3, thumbWidth, h.height - 6};
		if(app.nativeChrome) { if(!app.view.wrap && total > view.width) roundFill(app.horizontalThumb, .78); fill({app.width - 14, h.y, 14, 14}, .985, .988, .995); }
        else { fill(app.horizontalThumb, .72, .72, .72); fill({app.width - 18, h.y, 18, 18}, .94, .94, .94); }
		if(app.showStatus && !app.nativeChrome)
		{
			fill({0, app.height - 24, app.width, 24}, .94, .94, .94); fill({0, app.height - 24, app.width, 1}, .75, .75, .75);
			text(10, app.height - 7, "Ln " + std::to_string(doc.lineNumber()) + ", Col " + std::to_string(doc.column(doc.cursor) + 1), .2, 12);
			text(app.width - 285, app.height - 7, app.format.endingName(), .2, 12); text(app.width - 130, app.height - 7, app.format.encodingName(), .2, 12);
		}
	}
	void menus(Notepad& app)
	{
		fill({0, 0, app.width, 26}, .94, .94, .94); fill({0, 25, app.width, 1}, .75, .75, .75);
		const char* labels[] = {"File", "Edit", "Format", "View", "Help"}; int edges[] = {0, 46, 92, 158, 210, 260};
		for(int i = 0; i < 5; ++i)
		{
			if(app.menu == i) fill({edges[i], 1, edges[i + 1] - edges[i], 24}, .81, .86, .93);
			text(edges[i] + 9, 18, labels[i]);
		}
		if(app.menu < 0) return;
		auto items = app.menuItems(app.menu);
		app.menuRect = {edges[app.menu], 26, 286, static_cast<int>(items.size()) * 24 + 6}; auto r = app.menuRect;
		fill({r.x + 3, r.y + 3, r.width, r.height}, .65, .65, .65); fill(r, .97, .97, .97); border(r);
		for(size_t i = 0; i < items.size(); ++i)
		{
			auto item = items[i]; int y = r.y + 3 + static_cast<int>(i) * 24;
			if(item.command == Command::None) { fill({r.x + 30, y + 12, r.width - 36, 1}, .8, .8, .8); continue; }
			if(static_cast<int>(i) == app.menuSelection && item.enabled) fill({r.x + 2, y, r.width - 4, 24}, .81, .87, .95);
			if(item.checked) text(r.x + 9, y + 17, "x");
			text(r.x + 30, y + 17, item.title, item.enabled ? .08 : .6); text(r.x + 174, y + 17, item.shortcut, item.enabled ? .25 : .6, 12);
		}
	}
	void dialog(Notepad& app)
	{
		app.controls.clear(); app.fileListRect = {};
		if(app.dialog == Dialog::None) return;
		bool file = app.dialog == Dialog::Open || app.dialog == Dialog::Save;
		int w = std::min(550, app.width - 24), h = file ? std::min(372, app.height - 32) : app.dialog == Dialog::Replace ? 282 : 228;
		Rect r{(app.width - w) / 2, (app.height - h) / 2, w, h}; app.dialogRect = r;
		fill({r.x + 4, r.y + 4, r.width, r.height}, .6, .6, .6); fill(r, .94, .94, .94); border(r, .4); fill({r.x + 1, r.y + 1, r.width - 2, 30}, .17, .28, .46);
		std::string title = app.dialog == Dialog::Open ? "Open" : app.dialog == Dialog::Save ? "Save As" : app.dialog == Dialog::Unsaved ? "Editor" :
			app.dialog == Dialog::Overwrite ? "Confirm Save As" : app.dialog == Dialog::Find ? "Find" : app.dialog == Dialog::Replace ? "Replace" :
			app.dialog == Dialog::GoTo ? "Go To Line" : app.dialog == Dialog::Font ? "Font" : app.dialog == Dialog::About ? "About Editor" : "Editor";
		text(r.x + 10, r.y + 21, title, 1);
		int x = r.x + 18, y = r.y + 42, bottom = r.y + h - 44;
		if(file)
		{
			clippedText({x, y, w - 130, 22}, app.directory); button(app, {r.x + w - 96, y - 2, 78, 28}, "Up", Command::BrowseUp, 2);
			Rect list{x, y + 32, w - 36, h - 174}; app.fileListRect = list; fill(list, 1, 1, 1); border(list);
			cairo_save(cr); cairo_rectangle(cr, list.x + 1, list.y + 1, list.width - 2, list.height - 2); cairo_clip(cr);
			for(int i = app.fileTop; i < static_cast<int>(app.entries.size()) && i < app.fileTop + list.height / 22; ++i)
			{
				int rowY = list.y + (i - app.fileTop) * 22;
				if(i == app.fileSelected) fill({list.x + 1, rowY, list.width - 2, 22}, .81, .87, .95);
				clippedText({list.x + 7, rowY, list.width - 14, 22}, (app.entries[i].directory ? "[+] " : "    ") + app.entries[i].name);
			}
			cairo_restore(cr);
			if(app.entries.empty()) text(list.x + 10, list.y + 22, "No files in this folder", .5);
			text(x, r.y + h - 91, "File name:"); field(app, {x + 76, r.y + h - 113, w - 112, 32}, 0);
			clippedText({x, r.y + h - 77, w - 36, 26}, app.message, .25, 12);
			button(app, {r.x + w - 206, bottom, 90, 30}, app.dialog == Dialog::Open ? "Open" : "Save", Command::Accept, 0);
			button(app, {r.x + w - 108, bottom, 90, 30}, "Cancel", Command::Cancel, 1);
		}
		else if(app.dialog == Dialog::Find || app.dialog == Dialog::Replace)
		{
			text(x, y + 21, "Find what:"); field(app, {x + 104, y, w - 140, 32}, 0);
			if(app.dialog == Dialog::Replace) { text(x, y + 65, "Replace with:"); field(app, {x + 104, y + 44, w - 140, 32}, 1); }
			int optionsY = y + (app.dialog == Dialog::Replace ? 88 : 44);
			button(app, {x, optionsY, 144, 30}, app.matchCase ? "[x] Match case" : "[ ] Match case", Command::MatchCase, 0);
			clippedText({x, optionsY + 34, w - 36, 26}, app.message.empty() ? "Search wraps around the document." : app.message, .3, 12);
			if(app.dialog == Dialog::Replace)
			{
				button(app, {x, bottom, 112, 30}, "Find Next", Command::Accept, 1); button(app, {x + 120, bottom, 104, 30}, "Replace", Command::ReplaceOne, 2);
				button(app, {x + 232, bottom, 112, 30}, "Replace All", Command::ReplaceAll, 3); button(app, {r.x + w - 106, bottom, 88, 30}, "Cancel", Command::Cancel, 4);
			}
			else { button(app, {r.x + w - 228, bottom, 112, 30}, "Find Next", Command::Accept, 1); button(app, {r.x + w - 108, bottom, 90, 30}, "Cancel", Command::Cancel, 2); }
		}
		else if(app.dialog == Dialog::GoTo)
		{
			text(x, y + 21, "Line number:"); field(app, {x + 104, y, w - 140, 32}, 0); clippedText({x, y + 54, w - 36, 26}, app.message, .3, 12);
			button(app, {r.x + w - 206, bottom, 90, 30}, "Go To", Command::Accept, 0); button(app, {r.x + w - 108, bottom, 90, 30}, "Cancel", Command::Cancel, 1);
		}
		else if(app.dialog == Dialog::Unsaved)
		{
			wrappedText({x, y, w - 36, 80}, "Do you want to save changes to " + (app.path.empty() ? "Untitled" : baseName(app.path)) + "?");
			clippedText({x, bottom - 34, w - 36, 30}, app.message, .25, 12);
			button(app, {r.x + w - 314, bottom, 90, 30}, "Save", Command::ConfirmSave, 0); button(app, {r.x + w - 216, bottom, 108, 30}, "Don't Save", Command::Discard, 1);
			button(app, {r.x + w - 100, bottom, 82, 30}, "Cancel", Command::Cancel, 2);
		}
		else if(app.dialog == Dialog::Overwrite)
		{
			wrappedText({x, y, w - 36, 90}, "This file already exists. Replace it?");
			button(app, {r.x + w - 226, bottom, 110, 30}, "Replace", Command::ConfirmOverwrite, 0); button(app, {r.x + w - 108, bottom, 90, 30}, "Cancel", Command::Cancel, 1);
		}
		else if(app.dialog == Dialog::Font)
		{
			text(x, y + 22, "Consolas — " + std::to_string(app.fontSize) + " pt"); clippedText({x, y + 44, w - 36, 50}, "The quick brown fox jumps over the lazy dog.", .05, app.fontSize, true);
			button(app, {x, bottom, 100, 30}, "Smaller", Command::Smaller, 0); button(app, {x + 108, bottom, 100, 30}, "Larger", Command::Larger, 1);
			button(app, {r.x + w - 108, bottom, 90, 30}, "OK", Command::Accept, 2);
		}
		else
		{
			wrappedText({x, y, w - 36, 110}, app.dialog == Dialog::About ? "Ghost Editor\nA classic plain text editor.\nUTF-8 and UTF-16 • Windows and Unix line endings\nClipboard commands share text within this window." : app.message);
			button(app, {r.x + w - 108, bottom, 90, 30}, "OK", Command::Accept, 0);
		}
	}
	Rect paint(Notepad& app, bool caretOnly = false)
	{
		cairo_save(cr); cairo_set_font_face(cr, textFont); cairo_set_font_size(cr, app.fontSize);
		cairo_text_extents_t extents; cairo_text_extents(cr, "M", &extents);
		int oldCell = app.cell, oldLine = app.line;
		app.cell = std::max(1, static_cast<int>(std::ceil(extents.x_advance))); app.line = app.fontSize + 5; app.resize(app.width, app.height);
		if(oldCell != app.cell || oldLine != app.line) app.view.reveal(app.document);
		Rect damage{0, 0, app.width, app.height};
		caretOnly = caretOnly && app.nativeChrome && app.dialog == Dialog::None && app.menu < 0;
		if(caretOnly)
		{
			auto area = app.textRect();
			int x = area.x + (app.view.columnAt(app.document, app.document.cursor) - app.view.left) * app.cell;
			int y = area.y + (app.view.rowAt(app.document.cursor) - app.view.top) * app.line;
			damage = {std::max(x, area.x), std::max(y, area.y),
				std::max(0, std::min(x + 2, area.x + area.width) - std::max(x, area.x)),
				std::max(0, std::min(y + app.line - 2, area.y + area.height) - std::max(y, area.y))};
			cairo_rectangle(cr, damage.x, damage.y, damage.width, damage.height); cairo_clip(cr);
		}
		document(app, caretOnly);
		if(!app.nativeChrome) { menus(app); dialog(app); }
		cairo_restore(cr);
		return damage;
	}
};
}
#endif
