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

#ifndef STORAGEMANAGER_CONTROLS
#define STORAGEMANAGER_CONTROLS

#include <ghost/messages.h>
#include "registry/registry.hpp"

struct g_volume_pending
{
	g_tid client;
	g_message_transaction transaction;
	g_message_transaction driverTransaction;
	g_volume_id volume;
	g_tid handler;
	g_volume_command command;
	char name[32];
	uint64_t completed;
	uint64_t total;
};

struct g_volume_controls
{
	g_volume_registry* registry;
	std::map<g_message_transaction, g_volume_pending> pending;
};

/**
 * Checks disk availability and the requested mount name, forwarding the mount
 * to the filesystem handler when it is not already mounted.
 */
void volumeControlHandleMount(g_volume_controls* controls, g_tid sender, g_message_transaction tx, const g_volume_mount_request* request);

/**
 * Checks the optional expected claim and forwards unmounting to the handler
 * unless the volume is already available.
 */
void volumeControlHandleUnmount(g_volume_controls* controls, g_tid sender, g_message_transaction tx, const g_volume_unmount_request* request);

/**
 * Forwards a flush request to the filesystem handler and tracks its reply.
 */
void volumeControlHandleFlush(g_volume_controls* controls, g_tid sender, g_message_transaction tx, const g_volume_flush_request* request);

/**
 * Forwards a filesystem-space query and keeps it pending while scan progress
 * reports arrive from the handler.
 */
void volumeControlHandleStat(g_volume_controls* controls, g_tid sender, g_message_transaction tx, const g_volume_stat_request* request);

/**
 * Authenticates driver replies, relays valid STAT progress or final results
 * to the client and removes completed controls.
 */
void volumeControlHandleResponse(g_volume_controls* controls, const g_message_header* message, const g_volume_response* response);

/**
 * Applies pending-mount, name and shutdown rules before granting a worker's
 * volume claim.
 */
g_volume_response volumeControlHandleClaim(g_volume_controls* controls, g_tid sender, const g_volume_claim_request* request);

/**
 * Fails and removes pending controls whose filesystem handler has exited.
 */
void volumeControlReapPending(g_volume_controls* controls);

/**
 * Reports whether forwarded volume controls still await final driver replies.
 */
bool volumeControlHasPending(const g_volume_controls* controls);

/**
 * Rescans the requested disk when shutdown admission and disk reservations
 * permit it, returning the volume-protocol reply to the caller.
 */
g_volume_response volumeControlHandleRescan(g_volume_controls* controls, g_tid sender,
										  const g_volume_rescan_request* request);

#endif
