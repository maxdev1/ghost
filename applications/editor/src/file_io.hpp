#ifndef GHOST_EDITOR_FILE_IO_HPP
#define GHOST_EDITOR_FILE_IO_HPP

#include "document.hpp"
#include <cstdio>

namespace editor
{
enum class Encoding { Utf8, Utf8Bom, Utf16LE, Utf16BE };
enum class LineEnding { Windows, Unix, Mac };
struct FileFormat
{
	Encoding encoding = Encoding::Utf8;
	LineEnding ending = LineEnding::Windows;
	const char* encodingName() const
	{
		return encoding == Encoding::Utf8 ? "UTF-8" : encoding == Encoding::Utf8Bom ? "UTF-8 BOM" :
			encoding == Encoding::Utf16LE ? "UTF-16 LE" : "UTF-16 BE";
	}
	const char* endingName() const
	{
		return ending == LineEnding::Windows ? "Windows (CRLF)" : ending == LineEnding::Unix ? "Unix (LF)" : "Mac (CR)";
	}
};
inline bool decodeFile(const std::string& bytes, std::string& text, FileFormat& format, std::string& error)
{
	format = {};
	std::string decoded;
	if(bytes.size() >= 2 && ((static_cast<unsigned char>(bytes[0]) == 0xff && static_cast<unsigned char>(bytes[1]) == 0xfe) ||
		(static_cast<unsigned char>(bytes[0]) == 0xfe && static_cast<unsigned char>(bytes[1]) == 0xff)))
	{
		bool le = static_cast<unsigned char>(bytes[0]) == 0xff;
		format.encoding = le ? Encoding::Utf16LE : Encoding::Utf16BE;
		if(bytes.size() % 2) { error = "The file contains incomplete UTF-16 text."; return false; }
		auto unit = [&](size_t p) -> uint32_t
		{
			uint32_t a = static_cast<unsigned char>(bytes[p]), b = static_cast<unsigned char>(bytes[p + 1]);
			return le ? a | (b << 8) : (a << 8) | b;
		};
		for(size_t p = 2; p < bytes.size(); p += 2)
		{
			uint32_t c = unit(p);
			if(c >= 0xd800 && c <= 0xdbff)
			{
				if(p + 3 >= bytes.size() || unit(p + 2) < 0xdc00 || unit(p + 2) > 0xdfff)
				{ error = "The file contains invalid UTF-16 text."; return false; }
				c = 0x10000 + ((c - 0xd800) << 10) + unit(p + 2) - 0xdc00; p += 2;
			}
			else if(c >= 0xdc00 && c <= 0xdfff) { error = "The file contains invalid UTF-16 text."; return false; }
			appendUtf8(decoded, c);
		}
	}
	else if(bytes.compare(0, 3, "\xef\xbb\xbf") == 0)
	{
		format.encoding = Encoding::Utf8Bom; decoded = bytes.substr(3);
	}
	else decoded = bytes;
	if(!validUtf8(decoded)) { error = "Open UTF-8 or UTF-16 text files. Binary or legacy encoded text is unsupported."; return false; }
	if(decoded.find('\r') != std::string::npos)
		format.ending = decoded.find("\r\n") != std::string::npos ? LineEnding::Windows : LineEnding::Mac;
	else if(decoded.find('\n') != std::string::npos) format.ending = LineEnding::Unix;
	text = normalizeLines(decoded); return true;
}
inline std::string encodeFile(const std::string& text, const FileFormat& format)
{
	std::string lines;
	for(char c : text)
	{
		if(c == '\n') lines += format.ending == LineEnding::Windows ? "\r\n" : format.ending == LineEnding::Mac ? "\r" : "\n";
		else lines += c;
	}
	if(format.encoding == Encoding::Utf8) return lines;
	if(format.encoding == Encoding::Utf8Bom) return "\xef\xbb\xbf" + lines;
	bool le = format.encoding == Encoding::Utf16LE;
	std::string result = le ? "\xff\xfe" : "\xfe\xff";
	auto unit = [&](uint32_t c) { result += char(le ? c & 255 : c >> 8); result += char(le ? c >> 8 : c & 255); };
	size_t p = 0;
	uint32_t c;
	while(p < lines.size() && decodeUtf8(lines, p, c))
	{
		if(c <= 0xffff) unit(c);
		else { c -= 0x10000; unit(0xd800 | (c >> 10)); unit(0xdc00 | (c & 1023)); }
	}
	return result;
}
inline bool readFile(const std::string& path, std::string& text, FileFormat& format, std::string& error)
{
	FILE* file = fopen(path.c_str(), "rb");
	if(!file) { error = "Cannot open this file. Check the path and permissions."; return false; }
	std::string bytes;
	char buffer[8192];
	size_t count;
	bool ok = true;
	while((count = fread(buffer, 1, sizeof(buffer), file)) != 0)
	{
		bytes.append(buffer, count);
		if(bytes.size() > 16 * 1024 * 1024) { error = "This file exceeds the editor's 16 MiB limit."; ok = false; break; }
	}
	if(ferror(file)) { error = "The file could not be read completely."; ok = false; }
	if(fclose(file) != 0) { error = "Could not close the file after reading."; ok = false; }
	return ok && decodeFile(bytes, text, format, error);
}
inline bool writeFile(const std::string& path, const std::string& text, const FileFormat& format, std::string& error)
{
	auto bytes = encodeFile(text, format);
	FILE* file = fopen(path.c_str(), "wb");
	if(!file) { error = "Cannot save this file. Check the path and permissions."; return false; }
	size_t written = 0;
	while(written < bytes.size())
	{
		auto count = fwrite(bytes.data() + written, 1, bytes.size() - written, file);
		if(!count) break;
		written += count;
	}
	bool ok = written == bytes.size() && !ferror(file);
	if(fflush(file) != 0) ok = false;
	if(fclose(file) != 0) ok = false;
	if(!ok) error = "Save failed. Your changes are still in the editor; try Save As to another file.";
	return ok;
}
inline std::string resolvePath(const std::string& base, const std::string& input)
{
	std::string path = !input.empty() && input[0] == '/' ? input : base + "/" + input;
	std::vector<std::string> parts;
	for(size_t p = 0; p < path.size();)
	{
		auto end = path.find('/', p); if(end == std::string::npos) end = path.size();
		auto part = path.substr(p, end - p);
		if(part == "..") { if(!parts.empty()) parts.pop_back(); }
		else if(!part.empty() && part != ".") parts.push_back(part);
		p = end + 1;
	}
	std::string result;
	for(const auto& part : parts) result += "/" + part;
	return result.empty() ? "/" : result;
}
inline std::string parentPath(const std::string& path) { return resolvePath(path, ".."); }
inline std::string baseName(const std::string& path) { return path.substr(path.find_last_of('/') + 1); }
}
#endif
