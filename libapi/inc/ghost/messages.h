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
 * Returns the next transaction id that can be used for messaging.
 * When sending a message, a transaction can be added so that one can wait
 * for an answer with the same transaction. This method always returns a
 * new transaction id each time it is called and is thread-safe.
 *
 * @return a new transaction id
 *
 * @security-level APPLICATION
 */
g_message_transaction g_get_message_tx_id();

/**
 * Sends a message to the given task. This means that <len> bytes from the
 * buffer <buf> are copied to a message that is then sent to the <target> task.
 * The message may be no longer than {G_MESSAGE_MAXIMUM_MESSAGE_LENGTH}.
 *
 * The mode specifies how the function shall block:
 * - {G_MESSAGE_SEND_MODE_BLOCKING} the executing task will bock if the target tasks
 * 		message queue is full
 * - {G_MESSAGE_SEND_MODE_NON_BLOCKING} the function will return {G_MESSAGE_SEND_STATUS_FULL}
 * 		if the target tasks message queue is full
 *
 * @param target id of the target task
 * @param buf message content buffer
 * @param len number of bytes to copy from the buffer
 * @param-opt mode determines how the function blocks when given, default is {G_MESSAGE_SEND_MODE_BLOCKING}
 * @param-opt tx transaction id
 *
 * @return one of the <g_message_send_status> codes
 *
 * @security-level APPLICATION
 */
g_message_send_status g_send_message(g_tid target, void* buf, size_t len);
g_message_send_status g_send_message_m(g_tid target, void* buf, size_t len, g_message_send_mode mode);
g_message_send_status g_send_message_t(g_tid target, void* buf, size_t len, g_message_transaction tx);
g_message_send_status g_send_message_tm(g_tid target, void* buf, size_t len, g_message_transaction tx,
                                        g_message_send_mode mode);

/**
 * Receives a message. At maximum <max> bytes will be attempted to be copied to
 * the buffer <buf>. Note that when receiving a message, a buffer with a size of
 * at least the size of {g_message_header} plus the size of the sent message
 * must be used.
 *
 * After successful completion, the buffer will contain the message header followed
 * by the content of the message.
 * - to access the header, use the buffer pointer: ((g_message_header*) buf)
 * - to access the content, use the helper macro:  G_MESSAGE_CONTENT(buf)
 *
 * The mode specifies how the function shall block:
 * - {G_MESSAGE_RECEIVE_MODE_BLOCKING} the executing task will block if no messages
 * 		are available
 * - {G_MESSAGE_RECEIVE_MODE_NON_BLOCKING} the function will return {G_MESSAGE_RECEIVE_STATUS_EMPTY}
 * 		if the executing tasks message queue is empty
 *
 * When a transaction ID is given, only messages that were sent with the same
 * transaction ID will be received.
 *
 * @param buf output buffer
 * @param max maximum number of bytes to copy to the buffer
 * @param-opt mode determines how the function blocks when given, default is {G_MESSAGE_RECEIVE_MODE_BLOCKING}
 * @param-opt tx transaction id
 * @param-opt break_condition can be used to break the waiting process by setting its value to 1
 *
 * @security-level APPLICATION
 */
g_message_receive_status g_receive_message(void* buf, size_t max);
g_message_receive_status g_receive_message_m(void* buf, size_t max, g_message_receive_mode mode);
g_message_receive_status g_receive_message_t(void* buf, size_t max, g_message_transaction tx);
g_message_receive_status g_receive_message_tm(void* buf, size_t max, g_message_transaction tx,
                                              g_message_receive_mode mode);
g_message_receive_status g_receive_message_tmb(void* buf, size_t max, g_message_transaction tx,
                                               g_message_receive_mode mode, g_user_mutex break_condition);

/**
 * Sends a message to a topic.
 *
 * @param topic the target topic name
 * @param buf content buffer
 * @param len message length
 * @param-opt mode the send mode
 * @return send status
 */
g_message_send_status g_send_topic_message(const char* topic, void* buf, size_t len);
g_message_send_status g_send_topic_message_m(const char* topic, void* buf, size_t len, g_message_send_mode mode);

/**
 * Receives a message from a topic.
 *
 * @param topic the source topic name
 * @param buf output buffer
 * @param max maximum message length
 * @param start_after transaction number of the last received message or {G_MESSAGE_TOPIC_TRANSACTION_START}
 * @param-opt mode the reception mode
 * @return send status
 */
g_message_send_status g_receive_topic_message(const char* topic, void* buf, size_t max, g_message_transaction start_after);
g_message_send_status g_receive_topic_message_m(const char* topic, void* buf, size_t max, g_message_transaction start_after, g_message_receive_mode mode);

__END_C

#endif
