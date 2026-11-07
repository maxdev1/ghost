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

#ifndef GHOST_API_MESSAGES
#define GHOST_API_MESSAGES

#include "common.h"
#include "messages/types.h"
#include "mutex/types.h"

__BEGIN_C

/**
 * Allocates a transaction ID from the kernel's shared counter.
 *
 * Allocation is serialized across threads.
 *
 * @return A new transaction ID for correlating requests and replies.
 * @security-level APPLICATION
 */
g_message_transaction g_get_message_tx_id();

/**
 * Sends a message without a transaction ID, waiting for queue space.
 *
 * Equivalent to g_send_message_tm(target, buf, len, G_MESSAGE_TRANSACTION_NONE,
 * G_MESSAGE_SEND_MODE_BLOCKING).
 *
 * @param[in] target ID of the destination thread.
 * @param[in] buf Source payload containing at least len bytes; its contents are copied.
 * @param[in] len Payload size in bytes, at most G_MESSAGE_MAXIMUM_MESSAGE_LENGTH.
 * @return A g_message_send_status code; success means the message was queued, not processed.
 * @security-level APPLICATION
 */
g_message_send_status g_send_message(g_tid target, void* buf, size_t len);

/**
 * Sends a message without a transaction ID using the requested blocking mode.
 *
 * Equivalent to g_send_message_tm(target, buf, len, G_MESSAGE_TRANSACTION_NONE, mode).
 *
 * @param[in] target ID of the destination thread.
 * @param[in] buf Source payload containing at least len bytes; its contents are copied.
 * @param[in] len Payload size in bytes, at most G_MESSAGE_MAXIMUM_MESSAGE_LENGTH.
 * @param[in] mode G_MESSAGE_SEND_MODE_BLOCKING waits for queue space;
 *                 G_MESSAGE_SEND_MODE_NON_BLOCKING returns FULL instead.
 * @return A g_message_send_status code; success means the message was queued, not processed.
 * @security-level APPLICATION
 */
g_message_send_status g_send_message_m(g_tid target, void* buf, size_t len, g_message_send_mode mode);

/**
 * Sends a transaction-tagged message, waiting for queue space.
 *
 * Equivalent to g_send_message_tm(target, buf, len, tx, G_MESSAGE_SEND_MODE_BLOCKING).
 *
 * @param[in] target ID of the destination thread.
 * @param[in] buf Source payload containing at least len bytes; its contents are copied.
 * @param[in] len Payload size in bytes, at most G_MESSAGE_MAXIMUM_MESSAGE_LENGTH.
 * @param[in] tx Transaction ID copied into the message header, or G_MESSAGE_TRANSACTION_NONE.
 * @return A g_message_send_status code; success means the message was queued, not processed.
 * @security-level APPLICATION
 */
g_message_send_status g_send_message_t(g_tid target, void* buf, size_t len, g_message_transaction tx);

/**
 * Copies a payload into a destination thread's message queue.
 *
 * The kernel supplies the sender ID. A full queue blocks only in blocking mode; an absent
 * destination or oversized payload fails without queueing the message.
 *
 * @param[in] target ID of the destination thread.
 * @param[in] buf Source payload containing at least len bytes; its contents are copied.
 * @param[in] len Payload size in bytes, at most G_MESSAGE_MAXIMUM_MESSAGE_LENGTH.
 * @param[in] tx Transaction ID copied into the message header, or G_MESSAGE_TRANSACTION_NONE.
 * @param[in] mode G_MESSAGE_SEND_MODE_BLOCKING waits for queue space;
 *                 G_MESSAGE_SEND_MODE_NON_BLOCKING returns FULL instead.
 * @return A g_message_send_status code; success means the message was queued, not processed.
 * @security-level APPLICATION
 */
g_message_send_status g_send_message_tm(g_tid target, void* buf, size_t len, g_message_transaction tx,
                                        g_message_send_mode mode);

/**
 * Waits for and removes the next message from the calling thread's queue.
 *
 * Equivalent to g_receive_message_tm(buf, max, G_MESSAGE_TRANSACTION_NONE,
 * G_MESSAGE_RECEIVE_MODE_BLOCKING).
 *
 * @param[out] buf Destination for a g_message_header followed by its payload, with at least max
 *                 bytes of storage.
 * @param[in] max Total buffer capacity in bytes, including the message header.
 * @return A g_message_receive_status code; inspect the buffer only on
 *         G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL.
 * @security-level APPLICATION
 */
g_message_receive_status g_receive_message(void* buf, size_t max);

/**
 * Receives the next queued message using the requested blocking mode.
 *
 * Equivalent to g_receive_message_tm(buf, max, G_MESSAGE_TRANSACTION_NONE, mode).
 *
 * @param[out] buf Destination for a g_message_header followed by its payload, with at least max
 *                 bytes of storage.
 * @param[in] max Total buffer capacity in bytes, including the message header.
 * @param[in] mode G_MESSAGE_RECEIVE_MODE_BLOCKING waits for a matching message;
 *                 G_MESSAGE_RECEIVE_MODE_NON_BLOCKING returns EMPTY instead.
 * @return A g_message_receive_status code; inspect the buffer only on
 *         G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL.
 * @security-level APPLICATION
 */
g_message_receive_status g_receive_message_m(void* buf, size_t max, g_message_receive_mode mode);

/**
 * Waits for and removes a message matching a transaction ID.
 *
 * Equivalent to g_receive_message_tm(buf, max, tx, G_MESSAGE_RECEIVE_MODE_BLOCKING).
 *
 * @param[out] buf Destination for a g_message_header followed by its payload, with at least max
 *                 bytes of storage.
 * @param[in] max Total buffer capacity in bytes, including the message header.
 * @param[in] tx Required transaction ID, or G_MESSAGE_TRANSACTION_NONE to accept any transaction.
 * @return A g_message_receive_status code; inspect the buffer only on
 *         G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL.
 * @security-level APPLICATION
 */
g_message_receive_status g_receive_message_t(void* buf, size_t max, g_message_transaction tx);

/**
 * Receives a matching message from the calling thread's queue.
 *
 * On success, cast buf to g_message_header* and use G_MESSAGE_CONTENT(buf) for the payload. The
 * maximum-sized message needs sizeof(g_message_header) + G_MESSAGE_MAXIMUM_MESSAGE_LENGTH bytes. A
 * message exceeding max remains queued; unrelated transactions also remain queued.
 *
 * @param[out] buf Destination for a g_message_header followed by its payload, with at least max
 *                 bytes of storage.
 * @param[in] max Total buffer capacity in bytes, including the message header.
 * @param[in] tx Required transaction ID, or G_MESSAGE_TRANSACTION_NONE to accept any transaction.
 * @param[in] mode G_MESSAGE_RECEIVE_MODE_BLOCKING waits for a matching message;
 *                 G_MESSAGE_RECEIVE_MODE_NON_BLOCKING returns EMPTY instead.
 * @return A g_message_receive_status code; inspect the buffer only on
 *         G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL.
 * @security-level APPLICATION
 */
g_message_receive_status g_receive_message_tm(void* buf, size_t max, g_message_transaction tx,
                                              g_message_receive_mode mode);

/**
 * Receives a message with a legacy cancellation argument.
 *
 * Otherwise behaves as g_receive_message_tm(). The current kernel does not implement the break
 * condition, so it cannot interrupt a blocking receive.
 *
 * @param[out] buf Destination for a g_message_header followed by its payload, with at least max
 *                 bytes of storage.
 * @param[in] max Total buffer capacity in bytes, including the message header.
 * @param[in] tx Required transaction ID, or G_MESSAGE_TRANSACTION_NONE to accept any transaction.
 * @param[in] mode G_MESSAGE_RECEIVE_MODE_BLOCKING waits for a matching message;
 *                 G_MESSAGE_RECEIVE_MODE_NON_BLOCKING returns EMPTY instead.
 * @param[in] break_condition Legacy mutex handle; ignored by the current kernel. Pass zero.
 * @return A g_message_receive_status code; inspect the buffer only on
 *         G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL.
 * @security-level APPLICATION
 */
g_message_receive_status g_receive_message_tmb(void* buf, size_t max, g_message_transaction tx,
                                               g_message_receive_mode mode, g_user_mutex break_condition);

/**
 * Publishes a message to a topic using blocking send mode.
 *
 * Equivalent to g_send_topic_message_m(topic, buf, len, G_MESSAGE_SEND_MODE_BLOCKING).
 *
 * @param[in] topic Non-null, null-terminated topic name.
 * @param[in] buf Source payload containing at least len bytes; its contents are copied.
 * @param[in] len Payload size in bytes, at most G_MESSAGE_MAXIMUM_MESSAGE_LENGTH.
 * @return A g_message_send_status code.
 * @security-level APPLICATION
 */
g_message_send_status g_send_topic_message(const char* topic, void* buf, size_t len);

/**
 * Copies a message into a topic's shared message history.
 *
 * Creates the topic if needed. The kernel assigns the sender ID and a topic-local transaction
 * sequence number; readers advance independently through that history.
 *
 * @param[in] topic Non-null, null-terminated topic name.
 * @param[in] buf Source payload containing at least len bytes; its contents are copied.
 * @param[in] len Payload size in bytes, at most G_MESSAGE_MAXIMUM_MESSAGE_LENGTH.
 * @param[in] mode Requested G_MESSAGE_SEND_MODE_* behavior; current topic storage has no full
 *                 condition.
 * @return A g_message_send_status code.
 * @security-level APPLICATION
 */
g_message_send_status g_send_topic_message_m(const char* topic, void* buf, size_t len, g_message_send_mode mode);

/**
 * Waits for the next topic message after a caller-maintained cursor.
 *
 * Equivalent to g_receive_topic_message_m(topic, buf, max, start_after,
 * G_MESSAGE_RECEIVE_MODE_BLOCKING).
 *
 * @param[in] topic Non-null, null-terminated topic name.
 * @param[out] buf Destination for the message header and payload, with at least max bytes of
 *                 storage.
 * @param[in] max Total destination capacity in bytes, including g_message_header.
 * @param[in] start_after Last received topic transaction, or G_MESSAGE_TOPIC_TRANSACTION_START to
 *                        start at the oldest message.
 * @return A G_MESSAGE_RECEIVE_STATUS_* code.
 * @security-level APPLICATION
 */
g_message_receive_status g_receive_topic_message(const char* topic, void* buf, size_t max, g_message_transaction start_after);

/**
 * Reads a topic message after the supplied transaction without consuming it.
 *
 * On success, advance the cursor to the returned header's transaction. Other readers are
 * unaffected. The buffer contains g_message_header followed by its payload, accessible with
 * G_MESSAGE_CONTENT(buf). An undersized buffer returns
 * G_MESSAGE_RECEIVE_STATUS_EXCEEDS_BUFFER_SIZE.
 *
 * @param[in] topic Non-null, null-terminated topic name.
 * @param[out] buf Destination for the message header and payload, with at least max bytes of
 *                 storage.
 * @param[in] max Total destination capacity in bytes, including g_message_header.
 * @param[in] start_after Last received topic transaction, or G_MESSAGE_TOPIC_TRANSACTION_START to
 *                        start at the oldest message.
 * @param[in] mode G_MESSAGE_RECEIVE_MODE_BLOCKING waits for a matching message;
 *                 G_MESSAGE_RECEIVE_MODE_NON_BLOCKING returns EMPTY instead.
 * @return A G_MESSAGE_RECEIVE_STATUS_* code.
 * @security-level APPLICATION
 */
g_message_receive_status g_receive_topic_message_m(const char* topic, void* buf, size_t max, g_message_transaction start_after, g_message_receive_mode mode);

__END_C

#endif
