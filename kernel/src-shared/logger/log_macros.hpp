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

#ifndef GHOST_SHARED_LOGGER_LOGMACROS
#define GHOST_SHARED_LOGGER_LOGMACROS

#include <build_config.hpp>

// Info
#if G_LOG_LEVEL <= G_LOG_LEVEL_INFO
#define g_log_info(msg...)				g_logger::println(msg)
#define g_log_infon(msg...)				g_logger::print(msg)
#define G_LOGGING_INFO					true
#define G_IF_LOG_INFO(s)				s
#else
#define g_log_info(msg...)				{};
#define g_log_infon(msg...)				{};
#define G_LOGGING_INFO					false
#define G_IF_LOG_INFO(s)
#endif

// Warn
#if G_LOG_LEVEL <= G_LOG_LEVEL_WARN
#define g_log_warn(msg...)				g_logger::println(msg)
#define g_log_warnn(msg...)				g_logger::print(msg)
#define G_LOGGING_WARN					true
#define G_IF_LOG_WARN(s)				s
#else
#define g_log_warn(msg...)				{};
#define g_log_warnn(msg...)				{};
#define G_LOGGING_WARN					false
#define G_IF_LOG_WARN(s)
#endif

// Debug
#if G_LOG_LEVEL <= G_LOG_LEVEL_DEBUG
#define g_log_debug(msg...)				g_logger::println(msg)
#define g_log_debugn(msg...)			g_logger::print(msg)
#define G_LOGGING_DEBUG					true
#define G_IF_LOG_DEBUG(s)				s
#else
#define g_log_debug(msg...)				{};
#define g_log_debugn(msg...)			{};
#define G_LOGGING_DEBUG					false
#define G_IF_LOG_DEBUG(s)
#endif

#endif
