#ifndef NAVIGATOR_BROWSER_MODEL_HPP
#define NAVIGATOR_BROWSER_MODEL_HPP

#include <algorithm>
#include <string>
#include <vector>

namespace navigator
{
struct Entry
{
	std::string name;
	bool directory;
	bool file;
};

inline std::string folded(std::string value)
{
	for(auto& c : value)
		if(c >= 'A' && c <= 'Z') c += 'a' - 'A';
	return value;
}

inline std::string resolvePath(const std::string& base, const std::string& input)
{
	std::string path = input.empty() || input[0] != '/' ? base + "/" + input : input;
	std::vector<std::string> parts;
	for(size_t start = 0; start < path.size();)
	{
		auto end = path.find('/', start);
		if(end == std::string::npos) end = path.size();
		auto part = path.substr(start, end - start);
		if(part == "..") { if(!parts.empty()) parts.pop_back(); }
		else if(!part.empty() && part != ".") parts.push_back(part);
		start = end + 1;
	}
	std::string result;
	for(const auto& part : parts) result += "/" + part;
	return result.empty() ? "/" : result;
}

inline std::string displayName(const std::string& name, size_t maximum)
{
	size_t length = 0;
	for(unsigned char c : name) if((c & 0xc0) != 0x80) ++length;
	if(length <= maximum) return name;
	if(maximum <= 3) return std::string(maximum, '.');
	size_t end = 0;
	for(size_t count = 0; count < maximum - 3; ++count)
	{
		++end;
		while(end < name.size() && (static_cast<unsigned char>(name[end]) & 0xc0) == 0x80) ++end;
	}
	return name.substr(0, end) + "...";
}

inline std::string typeName(const Entry& entry)
{
	if(entry.directory) return "Folder";
	if(!entry.file) return "Device";
	auto dot = entry.name.find_last_of('.');
	if(dot == std::string::npos || dot == 0 || dot + 1 == entry.name.size()) return "File";
	auto extension = folded(entry.name.substr(dot + 1));
	if(extension == "bin") return "Application";
	return extension + " file";
}

inline std::vector<Entry> visibleEntries(const std::vector<Entry>& entries, const std::string& filter,
	bool hidden, bool byType, bool descending)
{
	std::vector<Entry> result;
	auto query = folded(filter);
	for(const auto& entry : entries)
		if(entry.name != "." && entry.name != ".." &&
			(hidden || entry.name.empty() || entry.name[0] != '.') &&
			folded(entry.name).find(query) != std::string::npos)
			result.push_back(entry);
	std::sort(result.begin(), result.end(), [=](const Entry& a, const Entry& b)
	{
		if(a.directory != b.directory) return a.directory;
		auto ak = byType ? typeName(a) : folded(a.name);
		auto bk = byType ? typeName(b) : folded(b.name);
		if(ak == bk) { ak = a.name; bk = b.name; }
		return descending ? ak > bk : ak < bk;
	});
	return result;
}

// Record only successful visits. Refreshes and repeated paths preserve Forward.
struct History
{
	std::vector<std::string> paths;
	int position = -1;
	bool canBack() const { return position > 0; }
	bool canForward() const { return position + 1 < static_cast<int>(paths.size()); }
	void record(const std::string& path)
	{
		if(position >= 0 && paths[position] == path) return;
		paths.erase(paths.begin() + (position + 1), paths.end());
		paths.push_back(path);
		position = static_cast<int>(paths.size()) - 1;
	}
};

// Ghost separates arguments with a unit separator; libc removes unescaped quotes/backslashes.
inline std::string argument(const std::string& value)
{
	std::string result;
	for(char c : value)
	{
		if(c == '\\' || c == '"') result += '\\';
		result += c;
	}
	return result;
}
}
#endif
