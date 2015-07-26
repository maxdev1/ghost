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

#ifndef __GHOST_LIBC_WCHAR__
#define __GHOST_LIBC_WCHAR__

#include "ghost/common.h"
#include <stddef.h>
#include <stdarg.h>

#include "file.h"

__BEGIN_C

// (N1548-7.28.1-2)
typedef struct {
	// TODO
} mbstate_t;

typedef __WINT_TYPE__ wint_t;

struct tm;

#define WEOF	0xFFFFFFFFu

// (N1548-7.28.2.1)
int fwprintf(FILE* stream, const wchar_t* format, ...);

// (N1548-7.28.2.2)
int fwscanf(FILE* stream, const wchar_t* format, ...);

// (N1548-7.28.2.3)
int swprintf(wchar_t* s, size_t n, const wchar_t* format, ...);

// (N1548-7.28.2.4)
int swscanf(const wchar_t* s, const wchar_t* format, ...);

// (N1548-7.28.2.5)
int vfwprintf(FILE* stream, const wchar_t* format, va_list arg);

// (N1548-7.28.2.6)
int vfwscanf(FILE* stream, const wchar_t* format, va_list arg);

// (N1548-7.28.2.7)
int vswprintf(wchar_t* s, size_t n, const wchar_t* format, va_list arg);

// (N1548-7.28.2.8)
int vswscanf(const wchar_t* s, const wchar_t* format, va_list arg);

// (N1548-7.28.2.9)
int vwprintf(const wchar_t* format, va_list arg);

// (N1548-7.28.2.10)
int vwscanf(const wchar_t* format, va_list arg);

// (N1548-7.28.2.11)
int wprintf(const wchar_t* format, ...);

// (N1548-7.28.2.12)
int wscanf(const wchar_t* format, ...);

// (N1548-7.28.3.1)
wint_t fgetwc(FILE* stream);

// (N1548-7.28.3.2)
wchar_t* fgetws(wchar_t* s, int n, FILE* stream);

// (N1548-7.28.3.3)
wint_t fputwc(wchar_t c, FILE* stream);

// (N1548-7.28.3.4)
int fputws(const wchar_t* s, FILE* stream);

// (N1548-7.28.3.5)
int fwide(FILE* stream, int mode);

// (N1548-7.28.3.6)
wint_t getwc(FILE* stream);

// (N1548-7.28.3.7)
wint_t getwchar();

// (N1548-7.28.3.8)
wint_t putwc(wchar_t c, FILE* stream);

// (N1548-7.28.3.9)
wint_t putwchar(wchar_t c);

// (N1548-7.28.3.10)
wint_t ungetwc(wint_t c, FILE* stream);

// (N1548-7.28.4.1.1)
double wcstod(const wchar_t* nptr, wchar_t** endptr);
float wcstof(const wchar_t* nptr, wchar_t** endptr);
long double wcstold(const wchar_t* nptr, wchar_t** endptr);

// (N1548-7.28.4.1.2)
long int wcstol(const wchar_t* nptr, wchar_t** endptr, int base);
long long int wcstoll(const wchar_t* nptr, wchar_t** endptr, int base);
unsigned long int wcstoul(const wchar_t* nptr, wchar_t** endptr, int base);
unsigned long long int wcstoull(const wchar_t* nptr, wchar_t** endptr,
		int base);

// (N1548-7.28.4.2.1)
wchar_t* wcscpy(wchar_t* s1, const wchar_t* s2);

// (N1548-7.28.4.2.2)
wchar_t* wcsncpy(wchar_t* s1, const wchar_t* s2, size_t n);

// (N1548-7.28.4.2.3)
wchar_t* wmemcpy(wchar_t* s1, const wchar_t* s2, size_t n);

// (N1548-7.28.4.2.4)
wchar_t* wmemmove(wchar_t* s1, const wchar_t* s2, size_t n);

// (N1548-7.28.4.3.1)
wchar_t* wcscat(wchar_t* s1, const wchar_t* s2);

// (N1548-7.28.4.3.2)
wchar_t* wcsncat(wchar_t* s1, const wchar_t* s2, size_t n);

// (N1548-7.28.4.4.1)
int wcscmp(const wchar_t* s1, const wchar_t* s2);

// (N1548-7.28.4.4.2)
int wcscoll(const wchar_t* s1, const wchar_t* s2);

// (N1548-7.28.4.4.3)
int wcsncmp(const wchar_t* s1, const wchar_t* s2, size_t n);

// (N1548-7.28.4.4.4)
size_t wcsxfrm(wchar_t* s1, const wchar_t* s2, size_t n);

// (N1548-7.28.4.4.5)
int wmemcmp(const wchar_t* s1, const wchar_t* s2, size_t n);

// (N1548-7.28.4.5.1)
wchar_t* wcschr(const wchar_t* s, wchar_t c);

// (N1548-7.28.4.5.2)
size_t wcscspn(const wchar_t* s1, const wchar_t* s2);

// (N1548-7.28.4.5.3)
wchar_t* wcspbrk(const wchar_t* s1, const wchar_t* s2);

// (N1548-7.28.4.5.4)
wchar_t* wcsrchr(const wchar_t* s, wchar_t c);

// (N1548-7.28.4.5.5)
size_t wcsspn(const wchar_t* s1, const wchar_t* s2);

// (N1548-7.28.4.5.6)
wchar_t* wcsstr(const wchar_t* s1, const wchar_t* s2);

// (N1548-7.28.4.5.7)
wchar_t* wcstok(wchar_t* s1, const wchar_t* s2, wchar_t** ptr);

// (N1548-7.28.4.5.8)
wchar_t* wmemchr(const wchar_t* s, wchar_t c, size_t n);

// (N1548-7.28.4.6.1)
size_t wcslen(const wchar_t* s);

// (N1548-7.28.4.6.2)
wchar_t* wmemset(wchar_t* s, wchar_t c, size_t n);

// (N1548-7.28.5.1)
size_t wcsftime(wchar_t* s, size_t maxsize, const wchar_t* format,
		const struct tm* timeptr);

// (N1548-7.28.6.1.1)
wint_t btowc(int c);

// (N1548-7.28.6.1.2)
int wctob(wint_t c);

// (N1548-7.28.6.2.1)
int mbsinit(const mbstate_t* ps);

// (N1548-7.28.6.3.1)
size_t mbrlen(const char* s, size_t n, mbstate_t* ps);

// (N1548-7.28.6.3.2)
size_t mbrtowc(wchar_t* pwc, const char* s, size_t n, mbstate_t* ps);

// (N1548-7.28.6.3.3)
size_t wcrtomb(char* s, wchar_t wc, mbstate_t* ps);

// (N1548-7.28.6.4.1)
size_t mbsrtowcs(wchar_t* dst, const char** src, size_t len, mbstate_t* ps);

// (N1548-7.28.6.4.2)
size_t wcsrtombs(char* dst, const wchar_t** src, size_t len, mbstate_t* ps);

__END_C

#endif
