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

#ifndef GHOST_FILESYSTEM_TRANSACTION_FINISH_HANDLER
#define GHOST_FILESYSTEM_TRANSACTION_FINISH_HANDLER

#include "ghost/stdint.h"
#include "tasking/thread.hpp"

class g_fs_delegate;

/**
 * Status for waiter
 */
typedef int g_fs_transaction_handler_finish_status;
const g_fs_transaction_handler_finish_status G_FS_TRANSACTION_HANDLING_DONE = 0;
const g_fs_transaction_handler_finish_status G_FS_TRANSACTION_HANDLING_REPEAT_WITH_SAME_HANDLER = 1;
const g_fs_transaction_handler_finish_status G_FS_TRANSACTION_HANDLING_KEEP_WAITING_WITH_NEW_HANDLER = 2;

/**
 *
 */
typedef int g_fs_transaction_handler_start_status;
const g_fs_transaction_handler_start_status G_FS_TRANSACTION_START_FAILED = 0;
const g_fs_transaction_handler_start_status G_FS_TRANSACTION_START_IMMEDIATE_FINISH = 1;
const g_fs_transaction_handler_start_status G_FS_TRANSACTION_START_WITH_WAITER = 2;

/**
 * A transaction handler is a temporary object that accompanies a transaction. It holds temporary values
 * that the (stateless) delegate needs for its operations. A handler is normally passed in two cases:
 *
 * - When something is requested from a delegate, the handler is passed so the delegate can
 *   immediately set the output values and call the finishing function if required
 *
 * - When a {g_waiter_fs_transaction} is notified that a transaction has finished, the delegate calls
 *   the finish function of the handler; the handler then calls the respective finish function of the
 *   delegate, so the delegate can fill the information that was the result of the transaction into
 *   the handler. Then the handler does anything that is further required, like filling a data structure
 *   or calling another function (see {g_fs_transaction_handler_discovery} etc.)
 */
class g_fs_transaction_handler {
private:
	g_fs_transaction_id repeat_transaction = G_FS_TRANSACTION_NO_REPEAT_ID;

public:
	virtual ~g_fs_transaction_handler() {
	}

	/**
	 *
	 */
	virtual g_fs_transaction_handler_start_status start_transaction(g_thread* thread) {
		return G_FS_TRANSACTION_START_IMMEDIATE_FINISH;
	}

	/**
	 *
	 */
	virtual g_fs_transaction_handler_finish_status finish_transaction(g_thread* thread, g_fs_delegate* delegate) = 0;

	/**
	 *
	 */
	bool wants_repeat_transaction() {
		return repeat_transaction != G_FS_TRANSACTION_NO_REPEAT_ID;
	}

	/**
	 *
	 */
	void prepare_transaction_repeat(g_fs_transaction_id tr) {
		this->repeat_transaction = tr;
	}

	/**
	 *
	 */
	g_fs_transaction_id get_repeated_transaction() {
		return this->repeat_transaction;
	}

};

#endif
