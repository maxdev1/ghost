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

#include "ghost/user.h"

g_spawn_status g_spawn(const char* path, const char* args, const char* workdir, g_security_level securityLevel)
{
	return g_spawn_poid(path, args, workdir, securityLevel, 0, 0, 0, 0);
}

g_spawn_status g_spawn_p(const char* path, const char* args, const char* workdir, g_security_level securityLevel, g_pid* pid)
{
	return g_spawn_poid(path, args, workdir, securityLevel, pid, 0, 0, 0);
}

g_spawn_status g_spawn_po(const char* path, const char* args, const char* workdir, g_security_level securityLevel, g_pid* pid, g_fd outStdio[3])
{
	return g_spawn_poid(path, args, workdir, securityLevel, pid, outStdio, 0, 0);
}

g_spawn_status g_spawn_poi(const char* path, const char* args, const char* workdir, g_security_level securityLevel, g_pid* pid, g_fd outStdio[3], g_fd inStdio[3])
{
	return g_spawn_poid(path, args, workdir, securityLevel, pid, outStdio, inStdio, 0);
}

g_spawn_status g_spawn_poid(const char* path, const char* args, const char* workdir, g_security_level securityLevel, g_pid* pid, g_fd outStdio[3],
		g_fd inStdio[3], g_spawn_validation_details* outValidationDetails)
{
	g_syscall_spawn data;
	data.path = (char*) path;
	data.args = args;
	data.workdir = workdir;
	data.securityLevel = securityLevel;

	if(inStdio)
	{
		data.inStdio[0] = inStdio[0];
		data.inStdio[1] = inStdio[1];
		data.inStdio[2] = inStdio[2];
	} else
	{
		data.inStdio[0] = -1;
		data.inStdio[1] = -1;
		data.inStdio[2] = -1;
	}

	g_syscall(G_SYSCALL_SPAWN, (g_address) &data);

	if(outStdio)
	{
		outStdio[0] = data.outStdio[0];
		outStdio[1] = data.outStdio[1];
		outStdio[2] = data.outStdio[2];
	}
	if(pid)
		*pid = data.pid;
	if(outValidationDetails)
		*outValidationDetails = data.validationDetails;
	return data.status;
}

