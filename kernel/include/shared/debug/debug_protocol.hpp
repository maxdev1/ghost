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

#ifndef __GHOST_DEBUG_PROTOCOL__
#define __GHOST_DEBUG_PROTOCOL__

#define G_DEBUG_MESSAGE_LOG 0x001

#define G_DEBUG_MESSAGE_TASK_SET_STATUS 0x101
#define G_DEBUG_MESSAGE_TASK_SET_IDENTIFIER 0x102
#define G_DEBUG_MESSAGE_TASK_SET_SOURCE_PATH 0x103
#define G_DEBUG_MESSAGE_TASK_SET_ROUNDS 0x104

#define G_DEBUG_MESSAGE_MEMORY_SET_PAGE_USAGE 0x201
#define G_DEBUG_MESSAGE_SYSTEM_INFORMATION 0x202

#define G_DEBUG_MESSAGE_FILESYSTEM_UPDATE_NODE 0x301

#endif
