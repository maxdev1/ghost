/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2026, Max Schlüssel <lokoxe@gmail.com>                     *
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

#include "filesystem.hpp"

static int filesystemToolFailure(const g_filesystem_tool_options& options, const char* action,
								 const std::string& path)
{
	filesystemToolError(options, action, path);
	return 2;
}

static void filesystemToolHelp(bool folder)
{
	printf("Usage: %s PATH ACTION [ARGUMENTS] [OPTIONS]\n", folder ? "folder" : "file");
	printf("  get                        Show metadata\n"
		   "  exists                     Test existence and type (exit 0 or 1)\n"
		   "  create                     Create an empty %s\n"
		   "  copy DESTINATION           Copy to an exact destination path\n"
		   "  move DESTINATION           Move to an exact path on the same filesystem\n"
		   "  delete                     Delete %s immediately, without prompting\n",
		   folder ? "directory" : "file", folder ? "the directory and all contents" : "the file");
	printf("  link TARGET                Create PATH as a new .link file atomically\n"
		   "  readlink                   Print its target without following it\n");
	if(folder)
		printf("  list [--long] [--recursive] List children, including dot-prefixed names\n"
			   "  tree [--depth N]           Show descendants as a hierarchy\n"
			   "  find PATTERN [--type file|folder|link]  Match descendant names with * and ?\n"
			   "  size                       Sum file bytes recursively\n"
			   "Options: create --parents creates missing parents and accepts an existing directory.\n");
	else
		printf("  read                       Print contents to standard output\n"
			   "  write                      Replace contents from standard input; create if missing\n"
			   "  append                     Append standard input; create if missing\n"
			   "  resize SIZE                Set byte length (bytes, KiB, MiB, GiB)\n"
			   "Options: --link-file reads/writes raw link storage for content operations.\n"
			   "Options: copy/move --replace permits replacing an existing regular file.\n");
	printf("Errors exit 2. Recursive operations do not cross mounts.\n");
}

static bool filesystemToolParse(int argc, char** argv, bool folder, g_filesystem_tool_options& options)
{
	if(argc < 3)
		return false;

	options.folder = folder;
	options.path = argv[1];
	options.action = argv[2];
	auto& action = options.action;
	bool common = action == "get" || action == "exists" || action == "create" || action == "copy" ||
				  action == "move" || action == "delete" || action == "link" || action == "readlink";
	bool specific = folder ? action == "list" || action == "tree" || action == "find" || action == "size"
						   : action == "read" || action == "write" || action == "append" || action == "resize";
	if(!common && !specific)
		return false;

	int cursor = 3;
	if(action == "copy" || action == "move" || action == "resize" || action == "find" || action == "link")
	{
		if(cursor == argc)
			return false;
		options.argument = argv[cursor++];
		if(options.argument.empty())
			return false;
	}

	while(cursor < argc)
	{
		std::string option = argv[cursor++];
		if(option == "--link-file" && !folder &&
		   (action == "read" || action == "write" || action == "append" || action == "resize" || action == "create") && !options.storage)
			options.storage = true;
		else if(option == "--replace" && !folder && (action == "copy" || action == "move") && !options.replace)
			options.replace = true;
		else if(option == "--parents" && folder && action == "create" && !options.parents)
			options.parents = true;
		else if(option == "--recursive" && folder && action == "list" && !options.recursive)
			options.recursive = true;
		else if(option == "--long" && folder && action == "list" && !options.detailed)
			options.detailed = true;
		else if(option == "--depth" && folder && action == "tree" && cursor < argc && options.depth == UINT_MAX)
		{
			uint64_t depth;
			if(!coreParseUnsigned(argv[cursor++], &depth, UINT_MAX - 1))
				return false;
			options.depth = depth;
		}
		else if(option == "--type" && folder && action == "find" && cursor < argc && options.type.empty())
		{
			options.type = argv[cursor++];
			if(options.type != "file" && options.type != "folder" && options.type != "link")
				return false;
		}
		else
			return false;
	}

	return !options.path.empty();
}

static bool filesystemToolCreateParents(const g_filesystem_tool_options& options, const std::string& path)
{
	std::string prefix;
	for(size_t i = 0; i <= path.size(); ++i)
	{
		if(i < path.size() && path[i] != '/')
		{
			prefix += path[i];
			continue;
		}

		if(!prefix.empty() && prefix != "/")
		{
			if(mkdir(prefix.c_str(), 0777) != 0)
			{
				struct stat info;
				if(errno != EEXIST || stat(prefix.c_str(), &info) != 0 || !S_ISDIR(info.st_mode))
					return filesystemToolError(options, "create", prefix);
			}
		}

		if(i < path.size())
			prefix += '/';
	}

	return true;
}

static bool filesystemToolSizeParse(const std::string& text, uint64_t& size)
{
	const char* cursor = text.c_str();
	if(!coreParseUnsignedPrefix(cursor, size, INT64_MAX))
		return false;

	uint64_t factor = 1;
	if(!strcmp(cursor, "KiB"))
		factor = 1024;
	else if(!strcmp(cursor, "MiB"))
		factor = 1024 * 1024;
	else if(!strcmp(cursor, "GiB"))
		factor = 1024ull * 1024 * 1024;
	else if(*cursor)
		return false;
	if(size > INT64_MAX / factor)
		return false;

	size *= factor;
	return true;
}

static int filesystemToolContents(const g_filesystem_tool_options& options)
{
	bool reading = options.action == "read";
	bool resizing = options.action == "resize";
	uint64_t size = 0;
	if(resizing && !filesystemToolSizeParse(options.argument, size))
	{
		fprintf(stderr, "file: invalid size '%s'\n", options.argument.c_str());
		return 2;
	}

	int flags = reading ? O_RDONLY : O_WRONLY;
	if(!reading && !resizing)
		flags |= O_CREAT;
	if(options.action == "append")
		flags |= O_APPEND;
	int descriptor = open(options.path.c_str(), flags | (options.storage ? O_LINKFILE : 0), 0666);
	if(descriptor < 0)
		return filesystemToolFailure(options, "open", options.path);

	struct stat info, input;
	bool success = true;
	if(fstat(descriptor, &info) != 0 || (!S_ISREG(info.st_mode) && !(options.storage && S_ISLNK(info.st_mode))))
	{
		errno = EINVAL;
		success = filesystemToolError(options, "use as a regular file", options.path);
	}
	if(success && !reading && !resizing)
	{
		if(fstat(STDIN_FILENO, &input) != 0)
			success = filesystemToolError(options, "inspect", "standard input");
		else if(input.st_dev == info.st_dev && input.st_ino == info.st_ino)
		{
			errno = EINVAL;
			success = filesystemToolError(options, "read its own input", options.path);
		}
	}

	if(success && (resizing || options.action == "write") && ftruncate(descriptor, size) != 0)
		success = filesystemToolError(options, "resize", options.path);
	if(success && !resizing)
		success = coreTransfer(reading ? descriptor : STDIN_FILENO, reading ? STDOUT_FILENO : descriptor,
			"file", reading ? options.path.c_str() : "standard input",
			reading ? "standard output" : options.path.c_str()) == 0;

	if(coreClose(descriptor, "file", options.path.c_str()))
		success = false;
	return success ? 0 : 2;
}

/**
 * Resolve the containing directory while preserving the final link entry.
 * Dot entries are resolved fully for ancestry checks and recursive deletion.
 */
static bool filesystemToolEntryAbsolute(const std::string& path, std::string& absolute)
{
	auto raw = path;
	while(raw.size() > 1 && raw.back() == '/') raw.pop_back();
	if(raw == "/") { absolute = "/"; return true; }
	auto slash = raw.find_last_of('/');
	auto name = slash == std::string::npos ? raw : raw.substr(slash + 1);
	if(name == "." || name == "..") return filesystemToolAbsolute(raw, absolute);
	auto parent = slash == std::string::npos ? "." : slash == 0 ? "/" : raw.substr(0, slash);
	std::string canonical;
	if(!filesystemToolAbsolute(parent, canonical)) return false;
	absolute = canonical + (canonical == "/" ? "" : "/") + name;
	return true;
}

static bool filesystemToolDestination(const g_filesystem_tool_options& options, std::string& destination)
{
	auto raw = options.argument;
	if(!filesystemToolPathValid(raw))
		return filesystemToolError(options, "use destination", raw);
	while(raw.size() > 1 && raw.back() == '/')
		raw.pop_back();
	auto slash = raw.find_last_of('/');
	auto name = slash == std::string::npos ? raw : raw.substr(slash + 1);
	if(name.empty() || name == "." || name == "..")
	{
		errno = EINVAL;
		return filesystemToolError(options, "use destination", raw);
	}

	std::string parent;
	if(!filesystemToolAbsolute(slash == std::string::npos ? "." : slash == 0 ? "/" : raw.substr(0, slash), parent))
		return filesystemToolError(options, "resolve destination parent", raw);
	destination = parent == "/" ? parent + name : parent + "/" + name;
	std::string source;
	if(!filesystemToolEntryAbsolute(options.path, source))
		return filesystemToolError(options, "resolve", options.path);
	bool insideSource = options.folder && (source == "/" ||
		destination.compare(0, source.size() + 1, source + "/") == 0);
	if(destination == source || insideSource)
	{
		errno = EINVAL;
		return filesystemToolError(options, "copy or move into itself", destination);
	}

	return true;
}

int filesystemToolRun(int argc, char** argv, bool folder)
{
	if((argc == 2 && coreArgumentIsHelp(argv[1])) || (argc == 4 && coreArgumentIsHelp(argv[3])))
	{
		filesystemToolHelp(folder);
		return 0;
	}

	g_filesystem_tool_options options;
	if(!filesystemToolParse(argc, argv, folder, options))
	{
		fprintf(stderr, "%s: invalid arguments; use --help\n", folder ? "folder" : "file");
		return 2;
	}

	if(options.action == "link")
		return symlink(options.argument.c_str(), options.path.c_str()) == 0 ? 0 : filesystemToolFailure(options, "create link", options.path);
	if(options.action == "readlink")
	{
		char target[G_PATH_MAX];
		auto length = readlink(options.path.c_str(), target, sizeof(target));
		if(length < 0) return filesystemToolFailure(options, "read link", options.path);
		fwrite(target, 1, length, stdout);
		putchar('\n');
		return 0;
	}

	g_filesystem_tool_info info;
	bool exists = filesystemToolInfoRead(options.path, info);
	bool query = options.action == "exists" || options.action == "get" || options.action == "list" ||
		options.action == "tree" || options.action == "find" || options.action == "size";
	if(folder && query && exists && S_ISLNK(info.metadata.st_mode))
		exists = stat(options.path.c_str(), &info.metadata) == 0;
	bool matches = exists && (folder ? S_ISDIR(info.metadata.st_mode) :
		(S_ISREG(info.metadata.st_mode) || S_ISLNK(info.metadata.st_mode)));
	if(options.action == "exists")
	{
		if(!exists && errno != ENOENT && errno != ENOTDIR)
			return filesystemToolFailure(options, "inspect", options.path);
		return matches ? 0 : 1;
	}

	bool mayCreate = options.action == "create" || options.action == "write" || options.action == "append";
	if((exists && !matches) || (!exists && (!mayCreate || errno != ENOENT)))
	{
		if(exists)
			errno = folder ? ENOTDIR : EINVAL;
		return filesystemToolFailure(options, "use requested type", options.path);
	}

	if(options.action == "create")
	{
		if(folder)
			return (options.parents ? filesystemToolCreateParents(options, options.path) :
					mkdir(options.path.c_str(), 0777) == 0 || filesystemToolError(options, "create", options.path)) ? 0 : 2;

		int descriptor = open(options.path.c_str(), O_WRONLY | O_CREAT | O_EXCL | (options.storage ? O_LINKFILE : 0), 0666);
		if(descriptor < 0)
			return filesystemToolFailure(options, "create", options.path);
		return coreClose(descriptor, "file", options.path.c_str()) ? 2 : 0;
	}

	if(options.action == "read" || options.action == "write" || options.action == "append" || options.action == "resize")
		return filesystemToolContents(options);
	if(options.action == "get")
	{
		std::string absolute;
		if(!filesystemToolEntryAbsolute(options.path, absolute))
			return filesystemToolFailure(options, "resolve", options.path);

		printf("Path: %s\nType: %s\n", absolute.c_str(), folder ? "folder" : S_ISLNK(info.metadata.st_mode) ? "link" : "file");
		if(!folder && info.metadata.st_size >= 0)
			printf("Size: %llu bytes\n", (unsigned long long) info.metadata.st_size);
		return 0;
	}

	if(options.action == "copy" || options.action == "move")
	{
		std::string destination;
		if(!filesystemToolDestination(options, destination))
			return 2;
		if(options.action == "copy")
			return filesystemToolCopy(options, options.path, destination) ? 0 : 2;

		int result = __g_fs_mutation_result_set_errno(g_rename(options.path.c_str(), destination.c_str(), options.replace));
		if(result != 0)
			return filesystemToolFailure(options, "move", options.path);
		return 0;
	}

	if(options.action == "delete")
	{
		std::string absolute;
		if(!filesystemToolEntryAbsolute(options.path, absolute))
			return filesystemToolFailure(options, "resolve", options.path);
		if(absolute == "/")
		{
			errno = EBUSY;
			return filesystemToolFailure(options, "delete root", absolute);
		}

		return filesystemToolDelete(options, absolute) ? 0 : 2;
	}

	uint64_t total = 0;
	if(!filesystemToolBrowse(options, options.path, "", 0, info.metadata.st_dev, total))
		return 2;
	if(options.action == "size")
		printf("%llu\n", (unsigned long long) total);
	return 0;
}

int main(int argc, char** argv)
{
	int result = filesystemToolRun(argc, argv, FILESYSTEM_TOOL_FOLDER);
	if(fflush(stdout) == EOF || ferror(stdout))
	{
		fprintf(stderr, "%s: cannot write standard output\n", FILESYSTEM_TOOL_FOLDER ? "folder" : "file");
		return 2;
	}

	return result;
}
