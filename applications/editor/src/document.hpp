// Ghost Editor — plain text editing, independent of the window server.
#ifndef GHOST_EDITOR_DOCUMENT_HPP
#define GHOST_EDITOR_DOCUMENT_HPP

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace editor
{
inline bool continuation(unsigned char c) { return (c & 0xc0) == 0x80; }
inline size_t next(const std::string& s, size_t p)
{
	if(p < s.size()) ++p;
	while(p < s.size() && continuation(s[p])) ++p;
	return p;
}
inline size_t previous(const std::string& s, size_t p)
{
	p = std::min(p, s.size());
	if(p) --p;
	while(p && continuation(s[p])) --p;
	return p;
}
inline void appendUtf8(std::string& s, uint32_t c)
{
	if(c < 0x80) s += static_cast<char>(c);
	else if(c < 0x800) { s += char(0xc0 | (c >> 6)); s += char(0x80 | (c & 63)); }
	else if(c < 0x10000)
	{
		s += char(0xe0 | (c >> 12)); s += char(0x80 | ((c >> 6) & 63)); s += char(0x80 | (c & 63));
	}
	else
	{
		s += char(0xf0 | (c >> 18)); s += char(0x80 | ((c >> 12) & 63));
		s += char(0x80 | ((c >> 6) & 63)); s += char(0x80 | (c & 63));
	}
}
inline bool decodeUtf8(const std::string& s, size_t& p, uint32_t& value)
{
	if(p >= s.size()) return false;
	unsigned char c = s[p++];
	if(c < 128) { value = c; return true; }
	int count = c >= 0xc2 && c <= 0xdf ? 1 : c >= 0xe0 && c <= 0xef ? 2 : c >= 0xf0 && c <= 0xf4 ? 3 : -1;
	if(count < 0 || p + count > s.size()) return false;
	value = c & (count == 1 ? 31 : count == 2 ? 15 : 7);
	for(int i = 0; i < count; ++i)
	{
		unsigned char byte = s[p++];
		if(!continuation(byte)) return false;
		value = (value << 6) | (byte & 63);
	}
	return value >= (count == 1 ? 0x80u : count == 2 ? 0x800u : 0x10000u) &&
		value <= 0x10ffff && !(value >= 0xd800 && value <= 0xdfff);
}
inline bool validUtf8(const std::string& s)
{
	size_t p = 0;
	uint32_t c;
	while(p < s.size()) if(!decodeUtf8(s, p, c) || c == 0) return false;
	return true;
}
inline std::string normalizeLines(const std::string& s)
{
	std::string result;
	for(size_t p = 0; p < s.size(); ++p)
	{
		if(s[p] == '\r') { result += '\n'; if(p + 1 < s.size() && s[p + 1] == '\n') ++p; }
		else result += s[p];
	}
	return result;
}
inline std::string fold(std::string s)
{
	for(char& c : s) if(c >= 'A' && c <= 'Z') c += 'a' - 'A';
	return s;
}
inline int advanceColumn(int column, char c) { return c == '\t' ? column + 8 - column % 8 : column + 1; }

class Document
{
	struct Edit
	{
		size_t position, beforeCursor, beforeAnchor, afterCursor;
		std::string removed, inserted;
	};
	std::vector<Edit> undoEdits, redoEdits;
	std::string saved;
	bool grouping = false;
	size_t revision_ = 0;

	void change(size_t start, size_t end, const std::string& value, bool join)
	{
		if(text.substr(start, end - start) == value) { move(start + value.size()); return; }
		Edit edit{start, cursor, anchor, start + value.size(), text.substr(start, end - start), value};
		// Consecutive typing is one undo step. Navigation, commands and newlines end the group.
		if(join && grouping && !undoEdits.empty() && redoEdits.empty() && start == end &&
			undoEdits.back().removed.empty() && undoEdits.back().afterCursor == start)
		{
			undoEdits.back().inserted += value;
			undoEdits.back().afterCursor += value.size();
		}
		else undoEdits.push_back(std::move(edit));
		text.replace(start, end - start, value);
		cursor = anchor = start + value.size();
		redoEdits.clear();
		grouping = join;
		++revision_;
	}
public:
	std::string text;
	size_t cursor = 0, anchor = 0;
	size_t revision() const { return revision_; }
	size_t first() const { return std::min(cursor, anchor); }
	size_t last() const { return std::max(cursor, anchor); }
	bool selected() const { return cursor != anchor; }
	std::string selection() const { return text.substr(first(), last() - first()); }
	bool dirty() const { return text != saved; }
	bool canUndo() const { return !undoEdits.empty(); }
	bool canRedo() const { return !redoEdits.empty(); }
	void endGroup() { grouping = false; }
	void markSaved() { saved = text; endGroup(); }
	void reset(std::string value = "")
	{
		text = std::move(value); cursor = anchor = 0;
		undoEdits.clear(); redoEdits.clear(); markSaved(); ++revision_;
	}
	void move(size_t p, bool extend = false)
	{
		cursor = std::min(p, text.size());
		while(cursor && cursor < text.size() && continuation(text[cursor])) --cursor;
		if(!extend) anchor = cursor;
		endGroup();
	}
	void selectAll() { anchor = 0; cursor = text.size(); endGroup(); }
	void insert(const std::string& value, bool typing = false)
	{
		auto normalized = normalizeLines(value);
		if(!validUtf8(normalized)) return;
		change(first(), last(), normalized, typing && !selected() && normalized.find('\n') == std::string::npos);
	}
	size_t lineStart(size_t p) const
	{
		if(!p) return 0;
		auto found = text.rfind('\n', p - 1);
		return found == std::string::npos ? 0 : found + 1;
	}
	size_t lineEnd(size_t p) const
	{
		auto found = text.find('\n', p);
		return found == std::string::npos ? text.size() : found;
	}
	int column(size_t p) const
	{
		int col = 0;
		for(auto i = lineStart(p); i < p; i = next(text, i)) col = advanceColumn(col, text[i]);
		return col;
	}
	size_t lineNumber() const { return 1 + std::count(text.begin(), text.begin() + cursor, '\n'); }
	size_t wordLeft(size_t p) const
	{
		while(p && whitespace(text[previous(text, p)])) p = previous(text, p);
		if(!p) return 0;
		auto kind = wordKind(text[previous(text, p)]);
		while(p && wordKind(text[previous(text, p)]) == kind) p = previous(text, p);
		return p;
	}
	size_t wordRight(size_t p) const
	{
		if(p < text.size())
		{
			auto kind = wordKind(text[p]);
			while(p < text.size() && wordKind(text[p]) == kind) p = next(text, p);
		}
		while(p < text.size() && whitespace(text[p])) p = next(text, p);
		return p;
	}
	static bool whitespace(char c) { return c == ' ' || c == '\t' || c == '\n'; }
	static int wordKind(unsigned char c)
	{
		return whitespace(c) ? 0 : (c >= 128 || (c >= 'a' && c <= 'z') ||
			(c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_') ? 1 : 2;
	}
	void horizontal(int direction, bool ctrl, bool extend)
	{
		if(selected() && !extend && !ctrl) move(direction < 0 ? first() : last());
		else move(direction < 0 ? (ctrl ? wordLeft(cursor) : previous(text, cursor)) :
			(ctrl ? wordRight(cursor) : next(text, cursor)), extend);
	}
	void erase(bool backwards, bool word = false)
	{
		auto start = first(), end = last();
		if(start == end)
		{
			if(backwards) start = word ? wordLeft(start) : previous(text, start);
			else end = word ? wordRight(end) : next(text, end);
		}
		if(start != end) change(start, end, "", false);
	}
	void undo()
	{
		if(!canUndo()) return;
		auto edit = std::move(undoEdits.back()); undoEdits.pop_back();
		text.replace(edit.position, edit.inserted.size(), edit.removed);
		cursor = edit.beforeCursor; anchor = edit.beforeAnchor;
		redoEdits.push_back(std::move(edit)); endGroup(); ++revision_;
	}
	void redo()
	{
		if(!canRedo()) return;
		auto edit = std::move(redoEdits.back()); redoEdits.pop_back();
		text.replace(edit.position, edit.removed.size(), edit.inserted);
		cursor = anchor = edit.afterCursor;
		undoEdits.push_back(std::move(edit)); endGroup(); ++revision_;
	}
	bool find(const std::string& needle, bool matchCase, bool backwards = false, bool fromCursor = false)
	{
		if(needle.empty()) return false;
		auto haystack = matchCase ? text : fold(text);
		auto query = matchCase ? needle : fold(needle);
		size_t start = fromCursor ? cursor : backwards ? first() : last();
		size_t found;
		if(backwards)
		{
			found = start ? haystack.rfind(query, start - 1) : std::string::npos;
			if(found == std::string::npos) found = haystack.rfind(query);
		}
		else
		{
			found = haystack.find(query, start);
			if(found == std::string::npos) found = haystack.find(query);
		}
		if(found == std::string::npos) return false;
		anchor = found; cursor = found + needle.size(); endGroup(); return true;
	}
	size_t replaceAll(const std::string& needle, const std::string& replacement, bool matchCase)
	{
		if(needle.empty()) return 0;
		auto haystack = matchCase ? text : fold(text), query = matchCase ? needle : fold(needle);
		std::string result;
		size_t start = 0, count = 0, found;
		while((found = haystack.find(query, start)) != std::string::npos)
		{
			result.append(text, start, found - start); result += replacement;
			start = found + query.size(); ++count;
		}
		if(count)
		{
			result.append(text, start, text.size() - start);
			change(0, text.size(), result, false);
		}
		return count;
	}
	bool goTo(size_t line)
	{
		if(line < 1) return false;
		size_t p = 0;
		while(--line)
		{
			auto found = text.find('\n', p);
			if(found == std::string::npos) return false;
			p = found + 1;
		}
		move(p); return true;
	}
};

struct Row { size_t start, end; int columns; };
class TextView
{
	size_t revision = static_cast<size_t>(-1);
	int layoutColumns = -1;
public:
	std::vector<Row> rows;
	int top = 0, left = 0, width = 80, height = 20, maxColumns = 0;
	bool wrap = false;
	void layout(const Document& document)
	{
		int columns = wrap ? std::max(1, width) : 0;
		if(revision == document.revision() && layoutColumns == columns) { clamp(); return; }
		revision = document.revision(); layoutColumns = columns;
		rows.clear(); maxColumns = 0;
		const auto& text = document.text;
		size_t start = 0, p = 0, lastSpace = 0;
		int col = 0;
		auto add = [&](size_t end)
		{
			rows.push_back({start, end, col}); maxColumns = std::max(maxColumns, col);
			start = end; col = 0; lastSpace = start;
		};
		while(p < text.size())
		{
			if(text[p] == '\n') { add(p); start = ++p; lastSpace = start; continue; }
			auto after = next(text, p);
			int nextCol = advanceColumn(col, text[p]);
			if(columns && nextCol > columns && p > start)
			{
				if(lastSpace > start)
				{
					p = lastSpace; col = 0;
					for(auto i = start; i < p; i = next(text, i)) col = advanceColumn(col, text[i]);
				}
				add(p); continue;
			}
			col = nextCol; p = after;
			if(text[after - 1] == ' ' || text[after - 1] == '\t') lastSpace = p;
		}
		add(text.size()); clamp();
	}
	int rowAt(size_t p) const
	{
		auto it = std::upper_bound(rows.begin(), rows.end(), p,
			[](size_t position, const Row& row) { return position < row.start; });
		return it == rows.begin() ? 0 : static_cast<int>(it - rows.begin()) - 1;
	}
	int columnAt(const Document& doc, size_t p) const
	{
		int col = 0;
		for(auto i = rows[rowAt(p)].start; i < p; i = next(doc.text, i)) col = advanceColumn(col, doc.text[i]);
		return col;
	}
	size_t position(const Document& doc, int row, int column) const
	{
		row = std::max(0, std::min(row, static_cast<int>(rows.size()) - 1));
		int col = 0;
		auto p = rows[row].start;
		while(p < rows[row].end)
		{
			int after = advanceColumn(col, doc.text[p]);
			if(column < col + (after - col + 1) / 2) break;
			col = after; p = next(doc.text, p);
		}
		return p;
	}
	void clamp()
	{
		top = std::max(0, std::min(top, std::max(0, static_cast<int>(rows.size()) - height)));
		left = wrap ? 0 : std::max(0, std::min(left, std::max(0, maxColumns + 1 - width)));
	}
	void reveal(const Document& doc)
	{
		int row = rowAt(doc.cursor), col = columnAt(doc, doc.cursor);
		if(row < top) top = row;
		if(row >= top + height) top = row - height + 1;
		if(col < left) left = col;
		if(col >= left + width) left = col - width + 1;
		clamp();
	}
};
}
#endif
