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

#ifndef GHOST_MULTITASKING_WAIT_MANAGER_FS_TRANSACTION
#define GHOST_MULTITASKING_WAIT_MANAGER_FS_TRANSACTION

#include "tasking/wait/waiter.hpp"

#include "filesystem/fs_transaction_handler.hpp"
#include "logger/logger.hpp"
#include "ghost/utils/local.hpp"
#include "utils/string.hpp"

/**
 * Waits for a specific transaction to be finished. Once the transaction is finished,
 * the given finish-handler is called (passing the task and the delegate) to do any further action.
 */
class g_waiter_fs_transaction: public g_waiter {
private:
	g_fs_transaction_handler* handler;
	g_fs_transaction_id transaction_id;
	g_fs_delegate* delegate;

public:
	/**
	 * Creates a transaction waiter.
	 *
	 * @param handler
	 * 		the handler to call when the transaction is finished. This handler will be deleted
	 * 		after the finish-transaction function was called
	 */
	g_waiter_fs_transaction(g_fs_transaction_handler* handler, g_fs_transaction_id transaction_id, g_fs_delegate* delegate) :
			handler(handler), transaction_id(transaction_id), delegate(delegate) {
	}

	/**
	 *
	 */
	virtual bool checkWaiting(g_thread* task) {
		return is_transaction_waiting(task, handler, transaction_id, delegate);
	}

	/**
	 * Used to check what to do with the given transaction.
	 */
	static bool is_transaction_waiting(g_thread* task, g_fs_transaction_handler* handler, g_fs_transaction_id transaction_id, g_fs_delegate* delegate) {

		g_fs_transaction_status status = g_fs_transaction_store::get_status(transaction_id);

		// transaction waits for something
		if (status == G_FS_TRANSACTION_WAITING) {
			return true;

			// same transaction shall be repeated (could not be fulfilled in the first attempt)
		} else if (status == G_FS_TRANSACTION_REPEAT) {

			// tell the handler that we will repeat
			handler->prepare_transaction_repeat(transaction_id);

			// repeat the transaction
			g_fs_transaction_handler_start_status restart_status = handler->start_transaction(task);

			if (restart_status != G_FS_TRANSACTION_START_FAILED) {
				// transaction retry successful - let it wait again
				return true;
			}

			// could not repeat transaction start, set it finished so it repeats once more and exits
			g_fs_transaction_store::set_status(status, G_FS_TRANSACTION_FINISHED);
			g_log_info("%! problem: failed to repeat a transaction");
			return true;

			// the transaction has finished
		} else if (status == G_FS_TRANSACTION_FINISHED) {

			// let the delegate do finish operations
			g_fs_transaction_handler_finish_status stat = handler->finish_transaction(task, delegate);

			// remove the transaction information
			g_fs_transaction_store::remove_transaction(transaction_id);

			// the handler has requested another action from the delegate & a new waiter was created and set
			if (stat == G_FS_TRANSACTION_HANDLING_REPEAT_WITH_SAME_HANDLER) {
				return true;

				// the transaction continues with a different handler, delete the old one
			} else if (stat == G_FS_TRANSACTION_HANDLING_KEEP_WAITING_WITH_NEW_HANDLER) {
				delete handler;
				return true;

				// the transaction is entirely finished and no more work must be done
			} else if (stat == G_FS_TRANSACTION_HANDLING_DONE) {
				delete handler;
				return false;

			}

			// the finish handler returned an unknown status. this is an error
			g_fs_transaction_store::remove_transaction(transaction_id);
			g_log_info("%! unknown finish handler status (%i) while waiting for transaction", "filesystem", stat);
			delete handler;
			return false;
		}

		// the transaction finished with an unknown status. this should never happen!
		g_fs_transaction_store::remove_transaction(transaction_id);
		g_log_info("%! unknown transaction status (%i) while waiting for transaction", "filesystem", status);
		delete handler;
		return false;
	}

	/**
	 *
	 */
	virtual const char* debug_name() {
		return "fs-transaction";
	}

};

#endif
