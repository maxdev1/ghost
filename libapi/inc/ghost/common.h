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

#ifndef __GHOST_COMMON__
#define __GHOST_COMMON__

/**
 * C++ compilers must know that the declarations are C declarations
 */
#ifdef __cplusplus
#define __BEGIN_C	extern "C" {
#define __END_C		}
#else
#define __BEGIN_C
#define __END_C
#endif

// standard declarations
#define __G_HAS_STDC11	(defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L)

// not implemented warning
#define __G_NOT_IMPLEMENTED_WARN(name)		g_log("'" #name "' is not implemented");
#define __G_NOT_IMPLEMENTED(name)		    __G_NOT_IMPLEMENTED_WARN(name) g_exit(0);

// debug tracing
#define __G_DEBUG_TRACE_ENABLED			0
#if __G_DEBUG_TRACE_ENABLED
#define __G_DEBUG_TRACE(function)		g_log(#function);
#else
#define __G_DEBUG_TRACE(function)
#endif

#endif
