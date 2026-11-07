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

#include "kernel/filesystem/filesystem.hpp"
#include "kernel/memory/heap.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/utils/string.hpp"

bool filesystemIsLinkName(const char* name)
{
	const char* end = name;
	while(*end)
		++end;
	if(end - name < 5)
		return false;

	end -= 5;
	return end[0] == '.' &&
	       (end[1] == 'l' || end[1] == 'L') &&
	       (end[2] == 'i' || end[2] == 'I') &&
	       (end[3] == 'n' || end[3] == 'N') &&
	       (end[4] == 'k' || end[4] == 'K');
}

g_fs_node_type filesystemStorageNodeType(g_fs_node_type type, const char* name)
{
	if(type == G_FS_NODE_TYPE_FILE || type == G_FS_NODE_TYPE_SYMLINK)
		return filesystemIsLinkName(name) ? G_FS_NODE_TYPE_SYMLINK : G_FS_NODE_TYPE_FILE;

	return type;
}

int32_t filesystemParseLink(const char* bytes, uint32_t length, char* target)
{
	if(length < G_FS_LINK_PREFIX_LENGTH || length > G_FS_LINK_STORAGE_MAX)
		return -1;

	const char prefix[] = "target=";
	for(uint32_t i = 0; i < G_FS_LINK_PREFIX_LENGTH; ++i)
		if(bytes[i] != prefix[i])
			return -1;

	if(bytes[length - 1] == '\n')
	{
		--length;
		if(bytes[length - 1] == '\r')
			--length;
	}

	auto count = length - G_FS_LINK_PREFIX_LENGTH;
	if(!count || count >= G_PATH_MAX)
		return -1;

	for(uint32_t i = 0; i < count; ++i)
	{
		auto value = (uint8_t) bytes[G_FS_LINK_PREFIX_LENGTH + i];
		if(value < 32 || value == 127)
			return -1;
		if(target)
			target[i] = value;
	}
	if(target)
		target[count] = 0;
	return count;
}

static bool filesystemIsDirectory(g_fs_node* node)
{
	return node && (node->type == G_FS_NODE_TYPE_FOLDER || node->type == G_FS_NODE_TYPE_ROOT ||
	               node->type == G_FS_NODE_TYPE_MOUNTPOINT);
}

g_fs_open_status filesystemReadLink(g_fs_node* node, char* target, uint32_t* length)
{
	*length = 0;
	g_mutex_guard guard(&node->lock);
	if(node->type != G_FS_NODE_TYPE_SYMLINK)
		return G_FS_OPEN_INVALID_LINK;

	auto delegate = filesystemFindDelegate(node);

	uint64_t size;
	if(!delegate->getLength || !delegate->read || delegate->getLength(node, &size) != G_FS_LENGTH_SUCCESSFUL)
		return G_FS_OPEN_ERROR;
	if(size > G_FS_LINK_STORAGE_MAX)
		return G_FS_OPEN_INVALID_LINK;

	auto bytes = (char*) heapTryAllocate(size ? size : 1);
	if(!bytes)
		return G_FS_OPEN_ERROR;

	uint64_t offset = 0;
	while(offset < size)
	{
		auto remaining = size - offset;
		int64_t count = -1;
		auto status = delegate->read(node, (uint8_t*) (bytes + offset), offset, remaining, &count);
		if(status != G_FS_READ_SUCCESSFUL || count <= 0 || (uint64_t) count > remaining)
		{
			heapFree(bytes);
			return G_FS_OPEN_ERROR;
		}
		offset += count;
	}

	auto parsed = filesystemParseLink(bytes, size, target);
	heapFree(bytes);
	if(parsed < 0)
		return G_FS_OPEN_INVALID_LINK;

	*length = parsed;
	return G_FS_OPEN_SUCCESSFUL;
}

static g_fs_open_status filesystemExpandLink(g_fs_node* node, const char* suffix, char* expanded)
{
	uint32_t length;
	auto status = filesystemReadLink(node, expanded, &length);
	if(status != G_FS_OPEN_SUCCESSFUL)
		return status;

	auto remaining = stringLength(suffix);
	if(length + remaining >= G_PATH_MAX)
		return G_FS_OPEN_NAME_TOO_LONG;

	memoryCopy(expanded + length, suffix, remaining + 1);
	return G_FS_OPEN_SUCCESSFUL;
}

/**
 * Walk components in order. Expanding a link replaces only the unconsumed
 * suffix, then restarts at its containing directory or the filesystem root.
 * No lexical '..' simplification occurs before following the preceding link.
 */
static g_fs_open_status filesystemWalkPath(g_fs_node* node, char* pending, char* expanded, bool followFinal,
                                          g_filesystem_find_result* result)
{
	unsigned followed = 0;
	auto cursor = pending;
	for(;;)
	{
		while(*cursor == '/')
			++cursor;
		if(!*cursor)
		{
			result->node = node;
			return G_FS_OPEN_SUCCESSFUL;
		}
		if(!filesystemIsDirectory(node))
			return G_FS_OPEN_NOT_DIRECTORY;

		auto end = cursor;
		while(*end && *end != '/')
			++end;

		auto count = (size_t) (end - cursor);
		if(count >= G_FILENAME_MAX)
			return G_FS_OPEN_NAME_TOO_LONG;

		memoryCopy(result->fileNameStart, cursor, count);
		result->fileNameStart[count] = 0;
		result->lastFoundParent = node;

		auto rest = end;
		while(*rest == '/')
			++rest;

		bool final = !*rest;
		bool trailingSlash = *end && final;
		g_fs_node* child = nullptr;
		auto status = filesystemFindChild(node, result->fileNameStart, &child);
		if(status != G_FS_OPEN_SUCCESSFUL)
		{
			result->foundAllButLast = final && !trailingSlash && status == G_FS_OPEN_NOT_FOUND;
			return status;
		}

		if(child->type == G_FS_NODE_TYPE_SYMLINK && (!final || followFinal))
		{
			if(++followed > G_FS_LINK_LIMIT)
				return G_FS_OPEN_LOOP;

			status = filesystemExpandLink(child, end, expanded);
			if(status != G_FS_OPEN_SUCCESSFUL)
				return status;

			node = expanded[0] == '/' ? filesystemGetRoot() : child->parent;
			auto old = pending;
			pending = expanded;
			expanded = old;
			cursor = pending;
			continue;
		}

		node = child;
		if(trailingSlash && !filesystemIsDirectory(node))
			return G_FS_OPEN_NOT_DIRECTORY;
		if(filesystemGetAbsolutePathLength(node) >= G_PATH_MAX)
			return G_FS_OPEN_NAME_TOO_LONG;

		cursor = end;
	}
}

g_filesystem_find_result filesystemFindPolicy(g_fs_node* parent, const char* path, bool followFinal)
{
	g_filesystem_find_result result{};
	result.status = G_FS_OPEN_NOT_FOUND;
	if(!path || !*path)
		return result;

	size_t size = 0;
	while(size < G_PATH_MAX && path[size])
		++size;
	if(size == G_PATH_MAX)
	{
		result.status = G_FS_OPEN_NAME_TOO_LONG;
		return result;
	}

	auto buffers = (char*) heapTryAllocate(2 * G_PATH_MAX);
	if(!buffers)
	{
		result.status = G_FS_OPEN_ERROR;
		return result;
	}

	memoryCopy(buffers, path, size + 1);
	auto origin = path[0] == '/' || !parent ? filesystemGetRoot() : parent;
	result.status = filesystemWalkPath(origin, buffers, buffers + G_PATH_MAX, followFinal, &result);

	heapFree(buffers);
	return result;
}

g_filesystem_find_result filesystemFind(g_fs_node* parent, const char* path)
{
	return filesystemFindPolicy(parent, path, true);
}

g_fs_open_status filesystemReadLinkPath(g_task* task, const char* path, char* target, uint32_t* length)
{
	g_fs_node* origin = nullptr;
	auto status = filesystemChooseOrigin(path, task, origin);
	if(status != G_FS_OPEN_SUCCESSFUL)
		return status;

	auto found = filesystemFindPolicy(origin, path, false);
	if(found.status != G_FS_OPEN_SUCCESSFUL)
		return found.status;

	return filesystemReadLink(found.node, target, length);
}

static g_fs_mutation_status filesystemCreateLinkTemporary(g_fs_node* parent, char* path, g_fs_node** created)
{
	char name[] = ".ghost-link-00000000";
	constexpr size_t prefixLength = sizeof(".ghost-link-") - 1;
	auto id = (uint32_t) filesystemGetNextNodeId();
	for(unsigned i = 0; i < 8; ++i)
		name[prefixLength + i] = "0123456789abcdef"[(id >> ((7 - i) * 4)) & 15];

	if(filesystemGetAbsolutePath(parent, path, G_PATH_MAX - sizeof(name)) < 0)
		return G_FS_MUTATION_NAME_TOO_LONG;

	auto offset = stringLength(path);
	if(offset > 1)
		path[offset++] = '/';
	stringCopy(path + offset, name);

	g_fs_node* collision = nullptr;
	auto status = filesystemFindChild(parent, name, &collision);
	if(status != G_FS_OPEN_NOT_FOUND)
		return status == G_FS_OPEN_SUCCESSFUL ? G_FS_MUTATION_ALREADY_EXISTS : filesystemMutationLookupStatus(status);

	status = filesystemFindDelegate(parent)->create(parent, name, created);
	return status == G_FS_OPEN_SUCCESSFUL ? G_FS_MUTATION_SUCCESSFUL : filesystemMutationLookupStatus(status);
}

static g_fs_mutation_status filesystemWriteLink(g_fs_node* node, char* bytes, uint64_t size)
{
	auto delegate = filesystemFindDelegate(node);
	constexpr auto flags = G_FILE_FLAG_MODE_WRITE | G_FILE_FLAG_MODE_LINK_STORAGE;
	if(delegate->open(node, flags) != G_FS_OPEN_SUCCESSFUL)
		return G_FS_MUTATION_ERROR;

	uint64_t written = 0;
	while(written < size)
	{
		auto remaining = size - written;
		int64_t count = -1;
		auto status = delegate->write(node, (uint8_t*) (bytes + written), written, remaining, &count);
		if(status != G_FS_WRITE_SUCCESSFUL || count <= 0 || (uint64_t) count > remaining)
			break;
		written += count;
	}

	auto status = written == size ? delegate->mutate(node, G_FS_DELEGATE_REQUEST_FLUSH, 0) : G_FS_MUTATION_ERROR;
	if(delegate->close(node, flags) != G_FS_CLOSE_SUCCESSFUL && status == G_FS_MUTATION_SUCCESSFUL)
		status = G_FS_MUTATION_ERROR;

	return status;
}

/**
 * Write and flush an ordinary temporary file before publishing it with a
 * non-replacing rename. Remove the temporary file if writing or publication fails.
 */
static g_fs_mutation_status filesystemPublishLink(g_task* task, const char* path, char* bytes, uint64_t size)
{
	g_fs_node* origin = nullptr;
	auto status = filesystemChooseOrigin(path, task, origin);
	if(status != G_FS_OPEN_SUCCESSFUL)
		return filesystemMutationLookupStatus(status);

	auto found = filesystemFindPolicy(origin, path, false);
	if(found.status == G_FS_OPEN_SUCCESSFUL)
		return G_FS_MUTATION_ALREADY_EXISTS;
	if(found.status != G_FS_OPEN_NOT_FOUND || !found.foundAllButLast)
		return filesystemMutationLookupStatus(found.status);
	if(!filesystemIsLinkName(found.fileNameStart))
		return G_FS_MUTATION_INVALID_ARGUMENT;

	auto delegate = filesystemFindDelegate(found.lastFoundParent);
	if(!delegate->create || !delegate->open || !delegate->close || !delegate->write || !delegate->mutate ||
	   !delegate->rename)
		return G_FS_MUTATION_NOT_SUPPORTED;

	auto temporary = (char*) heapTryAllocate(G_PATH_MAX);
	if(!temporary)
		return G_FS_MUTATION_ERROR;

	g_fs_node* created = nullptr;
	auto result = filesystemCreateLinkTemporary(found.lastFoundParent, temporary, &created);
	if(result == G_FS_MUTATION_SUCCESSFUL)
	{
		result = filesystemWriteLink(created, bytes, size);
		if(result == G_FS_MUTATION_SUCCESSFUL)
			result = filesystemRename(task, temporary, path, false);
		if(result != G_FS_MUTATION_SUCCESSFUL)
			delegate->mutate(created, G_FS_DELEGATE_REQUEST_REMOVE, 0);
	}

	heapFree(temporary);
	return result;
}

g_fs_mutation_status filesystemSymlink(g_task* task, const char* target, const char* path)
{
	size_t length = 0;
	while(length < G_PATH_MAX && target[length])
		++length;
	if(length == G_PATH_MAX)
		return G_FS_MUTATION_NAME_TOO_LONG;

	auto bytes = (char*) heapTryAllocate(G_FS_LINK_STORAGE_MAX);
	if(!bytes)
		return G_FS_MUTATION_ERROR;

	memoryCopy(bytes, "target=", G_FS_LINK_PREFIX_LENGTH);
	memoryCopy(bytes + G_FS_LINK_PREFIX_LENGTH, target, length);
	bytes[G_FS_LINK_PREFIX_LENGTH + length] = '\n';
	auto size = length + G_FS_LINK_PREFIX_LENGTH + 1;

	auto result = G_FS_MUTATION_INVALID_ARGUMENT;
	if(filesystemParseLink(bytes, size, nullptr) >= 0)
		result = filesystemPublishLink(task, path, bytes, size);

	heapFree(bytes);
	return result;
}
