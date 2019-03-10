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

#ifndef __LOGGER_MACROS__
#define __LOGGER_MACROS__

#include "build_config.hpp"

#if G_LOG_LEVEL <= G_LOG_LEVEL_INFO
#define logInfo(msg...)			loggerPrintlnLocked(msg)
#define logInfon(msg...)		loggerPrintLocked(msg)
#define G_LOGGING_INFO			true
#define G_IF_LOG_INFO(s)		s
#else
#define logInfo(msg...)			{};
#define logInfon(msg...)		{};
#define G_LOGGING_INFO			false
#define G_IF_LOG_INFO(s)
#endif

#if G_LOG_LEVEL <= G_LOG_LEVEL_WARN
#define logWarn(msg...)			loggerPrintlnLocked(msg)
#define logWarnn(msg...)		loggerPrintLocked(msg)
#define G_LOGGING_WARN			true
#define G_IF_LOG_WARN(s)		s
#else
#define logWarn(msg...)			{};
#define logWarnn(msg...)		{};
#define G_LOGGING_WARN			false
#define G_IF_LOG_WARN(s)
#endif

#if G_LOG_LEVEL <= G_LOG_LEVEL_DEBUG
#define logDebug(msg...)		loggerPrintlnLocked(msg)
#define logDebugn(msg...)		loggerPrintLocked(msg)
#define G_LOGGING_DEBUG			true
#define G_IF_LOG_DEBUG(s)		s
#else
#define logDebug(msg...)		{};
#define logDebugn(msg...)		{};
#define G_LOGGING_DEBUG			false
#define G_IF_LOG_DEBUG(s)
#endif

#endif
