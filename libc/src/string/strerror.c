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

#include "string.h"
#include "errno.h"

/**
 * Maps an error number to a static message without changing errno.
 */
char* strerror(int errnum)
{
	__G_DEBUG_TRACE(strerror);

	switch(errnum)
	{
		case 0:
			return "Success";
		case E2BIG:
			return "Argument list too long";
		case EACCES:
			return "Permission denied";
		case EADDRINUSE:
			return "Address already in use";
		case EADDRNOTAVAIL:
			return "Cannot assign requested address";
		case EAFNOSUPPORT:
			return "Address family not supported";
		case EAGAIN:
			return "Resource unavailable, try again";
		case EALREADY:
			return "Connection already in progress";
		case EBADF:
			return "Bad file descriptor";
		case EBADMSG:
			return "Bad message";
		case EBUSY:
			return "Device or resource busy";
		case ECANCELED:
			return "Operation canceled";
		case ECHILD:
			return "No child processes";
		case ECONNABORTED:
			return "Connection aborted";
		case ECONNREFUSED:
			return "Connection refused";
		case ECONNRESET:
			return "Connection reset";
		case EDEADLK:
			return "Resource deadlock would occur";
		case EDESTADDRREQ:
			return "Destination address required";
		case EDOM:
			return "Mathematics argument out of domain of function";
		case EDQUOT:
			return "Disk quota exceeded";
		case EEXIST:
			return "File exists";
		case EFAULT:
			return "Bad address";
		case EFBIG:
			return "File too large";
		case EHOSTUNREACH:
			return "Host is unreachable";
		case EIDRM:
			return "Identifier removed";
		case EILSEQ:
			return "Illegal byte sequence";
		case EINPROGRESS:
			return "Operation in progress";
		case EINTR:
			return "Interrupted function";
		case EINVAL:
			return "Invalid argument";
		case EIO:
			return "I/O error";
		case EISCONN:
			return "Socket is connected";
		case EISDIR:
			return "Is a directory";
		case ELOOP:
			return "Too many levels of symbolic links";
		case EMFILE:
			return "Too many open files";
		case EMLINK:
			return "Too many links";
		case EMSGSIZE:
			return "Message too large";
		case EMULTIHOP:
			return "Multihop attempted";
		case ENAMETOOLONG:
			return "Filename too long";
		case ENETDOWN:
			return "Network is down";
		case ENETRESET:
			return "Connection aborted by network";
		case ENETUNREACH:
			return "Network unreachable";
		case ENFILE:
			return "Too many files open in system";
		case ENOBUFS:
			return "No buffer space available";
		case ENODEV:
			return "No such device";
		case ENOENT:
			return "No such file or directory";
		case ENOEXEC:
			return "Executable file format error";
		case ENOLCK:
			return "No locks available";
		case ENOLINK:
			return "Link has been severed";
		case ENOMEM:
			return "Not enough memory";
		case ENOMSG:
			return "No message of the desired type";
		case ENOPROTOOPT:
			return "Protocol not available";
		case ENOSPC:
			return "No space left on device";
		case ENOSYS:
			return "Function not implemented";
		case ENOTCONN:
			return "Socket is not connected";
		case ENOTDIR:
			return "Not a directory";
		case ENOTEMPTY:
			return "Directory not empty";
		case ENOTRECOVERABLE:
			return "State not recoverable";
		case ENOTSOCK:
			return "Not a socket";
		case ENOTSUP:
			return "Operation not supported";
		case ENOTTY:
			return "Inappropriate I/O control operation";
		case ENXIO:
			return "No such device or address";
		case EOPNOTSUPP:
			return "Operation not supported on socket";
		case EOVERFLOW:
			return "Value too large to be stored in data type";
		case EOWNERDEAD:
			return "Previous owner died";
		case EPERM:
			return "Operation not permitted";
		case EPIPE:
			return "Broken pipe";
		case EPROTO:
			return "Protocol error";
		case EPROTONOSUPPORT:
			return "Protocol not supported";
		case EPROTOTYPE:
			return "Protocol wrong type for socket";
		case ERANGE:
			return "Result too large";
		case EROFS:
			return "Read-only file system";
		case ESPIPE:
			return "Invalid seek";
		case ESRCH:
			return "No such process";
		case ESTALE:
			return "Stale file handle";
		case ETIMEDOUT:
			return "Connection timed out";
		case ETXTBSY:
			return "Text file busy";
		case EWOULDBLOCK:
			return "Operation would block";
		case EXDEV:
			return "Cross-device link";
		default:
			return "Unknown error";
	}
}
