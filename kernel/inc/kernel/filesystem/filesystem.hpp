/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2015, Max Schlüssel <lokoxe@gmail.com>                     *
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

#ifndef __KERNEL_FILESYSTEM__
#define __KERNEL_FILESYSTEM__

#include "ghost/fs.h"
#include "kernel/tasking/tasking.hpp"
#include "shared/system/mutex.hpp"

struct g_fs_node;
struct g_fs_node_entry;
struct g_fs_delegate;

/**
 * A node on the virtual file system.
 */
struct g_fs_node
{
	g_fs_virt_id id;
	g_fs_phys_id physicalId;
	g_fs_node_type type;

	char* name;
	g_fs_node* parent;
	g_fs_node_entry* children;

	g_fs_delegate* delegate;

	bool blocking;
	bool upToDate;
};

/**
 * An entry in the node tree.
 */
struct g_fs_node_entry
{
	g_fs_node* node;
	g_fs_node_entry* next;
};

/**
 * A file system delegate.
 */
struct g_fs_delegate
{
	g_mutex lock;

	g_fs_open_status (*open)(g_fs_node* node);
	g_fs_open_status (*discover)(g_fs_node* parent, const char* name, g_fs_node** outNode);
	g_fs_read_status (*read)(g_fs_node* node, uint8_t* buffer, uint64_t offset, uint64_t length, int64_t* outRead);
	g_fs_write_status (*write)(g_fs_node* node, uint8_t* buffer, uint64_t offset, uint64_t length, int64_t* outWrote);
	g_fs_length_status (*getLength)(g_fs_node* node, uint64_t* outLength);
	g_fs_open_status (*create)(g_fs_node* parent, const char* name, g_fs_node** outFile);
	g_fs_open_status (*truncate)(g_fs_node* file);
	g_fs_close_status (*close)(g_fs_node* node);

	/**
	 * When resolvers used when a task needs to wait for a file.
	 */
	bool (*waitResolverRead)(g_task* task);
	bool (*waitResolverWrite)(g_task* task);
};

/**
 * Initializes the basic file system structures.
 */
void filesystemInitialize();

/**
 * Creates the filesystem root and mounts the ramdisk.
 */
void filesystemCreateRoot();

/**
 * Returns the next free virtual node id.
 */
g_fs_virt_id filesystemGetNextNodeId();

/**
 * Creates a node in the virtual file system.
 */
g_fs_node* filesystemCreateNode(g_fs_node_type type, const char* name);

/**
 * Searches for an existing node in the parent, otherwise asks the delegate of the
 * parent for this child.
 */
g_fs_open_status filesystemFindChild(g_fs_node* parent, const char* name, g_fs_node** outChild);

/**
 * Searches for a node by a path relative to the parent node. Uses filesystemFindChild
 * to search for a child.
 */
g_fs_open_status filesystemFind(g_fs_node* parent, const char* path, g_fs_node** outChild, bool* outFoundAllButLast = 0, g_fs_node** outLastFoundNode = 0,
		const char** outFileNameStart = 0);

/**
 * Retrieves a node by its virtual id.
 */
g_fs_node* filesystemGetNode(g_fs_virt_id id);

/**
 * Adds a child node to a parent.
 */
void filesystemAddChild(g_fs_node* parent, g_fs_node* child);

/**
 * Creates an empty file system delegate. This delegate can then be filled with handler
 * functions and appended to a node. For all requests on child nodes that have no delegate,
 * it searches up in the tree for a node with a delegate to perform those requests.
 */
g_fs_delegate* filesystemCreateDelegate();

/**
 * Returns the file system root.
 */
g_fs_node* filesystemGetRoot();

/**
 * Searches for the delegate responsible for this node.
 */
g_fs_delegate* filesystemFindDelegate(g_fs_node* node);

/**
 * Opens a file, creating a file descriptor.
 */
g_fs_open_status filesystemOpen(const char* path, g_file_flag_mode flags, g_task* task, g_fd* outFd);
g_fs_open_status filesystemOpen(g_fs_node* file, g_file_flag_mode flags, g_task* task, g_fd* outFd);

/**
 * Reads bytes from a file.
 */
g_fs_read_status filesystemRead(g_task* task, g_fd fd, uint8_t* buffer, uint64_t length, int64_t* outRead);
g_fs_read_status filesystemRead(g_fs_node* file, uint8_t* buffer, uint64_t offset, uint64_t length, int64_t* outRead);

/**
 * Writes bytes to a file.
 */
g_fs_write_status filesystemWrite(g_task* task, g_fd fd, uint8_t* buffer, uint64_t length, int64_t* outWrote);
g_fs_write_status filesystemWrite(g_fs_node* file, uint8_t* buffer, uint64_t offset, uint64_t length, int64_t* outWrote);

/**
 * Closes a file descriptor.
 */
g_fs_close_status filesystemClose(g_pid process, g_fd fd, g_bool removeDescriptor);

/**
 * Seeks in a file.
 */
g_fs_seek_status filesystemSeek(g_task* task, g_fd fd, g_fs_seek_mode mode, int64_t amount, int64_t* outResult);

/**
 * Retrieves the length of a file.
 */
g_fs_length_status filesystemGetLength(g_fs_node* file, uint64_t* outLength);

/**
 * Creates a file.
 */
g_fs_open_status filesystemCreateFile(g_fs_node* parent, const char* name, g_fs_node** outFile);

/**
 * Truncates a file.
 */
g_fs_open_status filesystemTruncate(g_fs_node* file);

/**
 * The task just tried to write to this file but the file was busy. This puts a waiter on the task
 * which lets the task wait until it can write to the file again.
 */
void filesystemWaitToWrite(g_task* task, g_fs_node* file);

/**
 * The task just tried to read from this file but the file was busy. This puts a waiter on the task
 * which lets the task wait until it can read from the file again.
 */
void filesystemWaitToRead(g_task* task, g_fs_node* file);

/**
 * Creates a new pipe on the filesystem.
 */
g_fs_pipe_status filesystemCreatePipe(g_bool blocking, g_fs_node** outPipeNode);

/**
 * Writes the absolute path of node into the given buffer (which must be of G_PATH_MAX bytes size).
 * 
 * @return the number of bytes written
 */
int filesystemGetAbsolutePath(g_fs_node* node, char* buffer);

/**
 * Calculates the absolute path length of node.
 * 
 * @return the path length
 */
int filesystemGetAbsolutePathLength(g_fs_node* node);

#endif
