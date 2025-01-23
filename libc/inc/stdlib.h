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

#ifndef __GHOST_LIBC_STDLIB__
#define __GHOST_LIBC_STDLIB__

#include "malloc.h"

__BEGIN_C

// (N1548-7.22-2)
typedef struct {
	int quot;
	int rem;
} div_t;

typedef struct {
	long quot;
	long rem;
} ldiv_t;

typedef struct {
	long long quot;
	long long rem;
} lldiv_t;

// (N1548-7.22-3)
#define EXIT_FAILURE	1
#define EXIT_SUCCESS	0

#define RAND_MAX		(0x7fffffff)

#define MB_CUR_MAX		((size_t) 4)

/**
 * Converts the given string to a double. (N1548-7.22.1.1)
 *
 * @param s
 * 		string to convert
 * @return
 * 		converted value
 */
double atof(const char* s);

/**
 * Converts the given string to a value of the respective integer type. (N1548-7.22.1.2)
 *
 * @param str
 * 		string to convert
 * @return
 * 		converted value
 */
int atoi(const char* str);
long atol(const char* str);
long long atoll(const char* str);

/**
 * Converts the given string to a value of the respective decimal type. (N1548-7.22.1.3)
 *
 * @param str
 * 		string to convert
 * @param endptr
 * 		is set to the end of the numeric value in <str>
 * @return
 * 		converted value
 */
float strtof(const char* str, char** endptr);
double strtod(const char* str, char** endptr);
long double strtold(const char* str, char** endptr);

/**
 * Converts the given string to a value of the respective integer type. (N1548-7.22.1.4)
 *
 * @param str
 * 		string to convert
 * @param endptr
 * 		is set to the end of the numeric value in <str>
 * @param base
 * 		conversion base
 * @return
 * 		converted value
 */
long strtol(const char* str, char** endptr, int base);
unsigned long strtoul(const char* str, char** endptr, int base);
long long strtoll(const char* str, char** endptr, int base);
unsigned long long strtoull(const char* str, char** endptr, int base);

/**
 * Computes a pseudo-random integer value in the range from 0 to <RAND_MAX>. (N1548-7.22.2.1)
 *
 * @return
 * 		computed pseudo-random value
 */
int rand();

/**
 * Computes a pseudo-random integer value in the range from 0 to <RAND_MAX>,
 * using the <seed> value for better randomization. (N1548-7.22.2.2)
 *
 * @param seed
 * 		a seed value
 * @return
 * 		computed pseudo-random value
 */
void srand(unsigned int seed);

/**
 * Terminates the program in an abnormal way. (N1548-7.22.4.1)
 */
void abort() __attribute__((noreturn));

/**
 * Registers the given function to be executed without arguments
 * at normal program termination. (N1548-7.22.4.2)
 *
 * @param func
 * 		function to register
 * @return
 * 		zero if successful, otherwise non-zero
 */
int atexit(void (*func)(void));

/**
 * Registers the given function to be executed without arguments
 * at program termination with <quick_exit>. (N1548-7.22.4.3)
 *
 * @param func
 * 		function to register
 * @return
 * 		zero if successful, otherwise non-zero
 */
int at_quick_exit(void (*func)(void));

/**
 * Terminates the program in a normal way, passing the exit <code>
 * to the environment. Calls handlers registered with <atexit>. (N1548-7.22.4.4)
 *
 * @param code
 * 		exit code to pass
 */
void exit(int code) __attribute__((noreturn));

/**
 * Equivalent to a call to <exit>. (N1548-7.22.4.5)
 *
 * @param code
 * 		exit code to pass
 */
void _Exit(int code) __attribute__((noreturn));

/**
 * Terminates the program in a normal way, passing the exit <code> to the environment.
 * Calls handlers registered with <at_quick_exit>. (N1548-7.22.4.7)
 */
void quick_exit(int) __attribute__((noreturn));

/**
 * Returns the value of the environment variable <key>. (N1548-7.22.4.6)
 *
 * @param key
 * 		variable key
 * @return
 * 		pointer to the value
 */
char* getenv(const char* key);

/**
 * Sets the environment variable <key> to <value>. If overwrite is false,
 * the variable is not updated if already existing and only created if not
 * existing. If overwrite is true, the variable is set. (POSIX)
 *
 * @param key
 * 		variable key
 * @param value
 * 		value to set
 * @param overwrite
 * 		whether to overwrite ane existing variable
 * @return
 * 		zero if successful, otherwise -1
 */
int setenv(const char* key, const char* value, int overwrite);

/**
 * Executes the given <command> with the system command processor. (N1548-7.22.4.8)
 *
 * @param command
 * 		command to process
 * @return
 * 		return code of the command processor
 */
int system(const char* command);

/**
 * Parses the command line arguments.
 *
 * @param out_argc
 * 		is filled with the number of arguments
 * @param out_args
 * 		is assigned with the argument array
 * @return
 * 		zero if successful, otherwise non-zero
 */
int parseargs(int* out_argc, char*** out_args);

/**
 * Binary search for the <value> in the <array>. The array consists of <num_elements> elements
 * with a size of <size> each. The <comparator> is hereby used to compare two of these elements and
 * must return a value less than, equal to, or greater than zero if the first argument is considered
 * to be respectively less than, equal to, or greater than the second. (N1548-7.22.5.1)
 *
 * @param value
 * 		value to find
 * @param array
 * 		array to search
 * @param num_elements
 * 		number of elements in the array
 * @param size
 * 		size of each element
 * @param comparator
 * 		comparator to use
 * @return
 * 		the found element, or NULL
 */
void* bsearch(const void* value, const void* array, size_t num_elements,
		size_t size, int (*comparator)(const void*, const void*));

/**
 * Performs a quick-sort on the <array>. The array consists of <num_elements> elements
 * with a size of <size> each. The <comparator> is hereby used to compare two of these elements and
 * must return a value less than, equal to, or greater than zero if the first argument is considered
 * to be respectively less than, equal to, or greater than the second. (N1548-7.22.5.2)
 *
 * @param array
 * 		array to sort
 * @param num_elements
 * 		number of elements in the array
 * @param size
 * 		size of each element
 * @param comparator
 * 		comparator to use
 */
void qsort(void* array, size_t num_elements, size_t size,
		int (*compar)(const void *, const void *));

/**
 * Calculates the absolute value of each respective integer type value. (N1548-7.22.6.1)
 *
 * @param val
 * 		value to calculate
 * @return
 * 		calculated absolute value
 */
int abs(int val);
long labs(long val);
long long llabs(long long val);

/**
 * Computes the <numer> / <denom> and <numer> % <denom> of each respective integer
 * type value in a single operation. (N1548-7.22.6.2)
 *
 * @param numer
 * 		value to divide
 * @param denom
 * 		divisor to use
 * @return
 * 		a value of type <div_t> containing the calculation result
 */
div_t div(int numer, int denom);
ldiv_t ldiv(long int numer, long int denom);
lldiv_t lldiv(long long int numer, long long int denom);

/**
 * Returns the number of bytes contained in the multibyte character <s>. (N1548-7.22.7.1)
 *
 * @param s
 * 		string containing the multibyte character
 * @param n
 * 		number of bytes to search for the character
 * @return
 * 		non-zero if multibyte character encodings have state-dependent encodings,
 * 		otherwise zero
 */
int mblen(const char* s, size_t n);

/**
 * Converts the multi-byte character in <s> (inspecting at most <n> bytes) and
 * writes the wide-character value to <pwc> (if not NULL). (N1548-7.22.7.2)
 *
 * @param pwc
 * 		output location
 * @param s
 * 		source string
 * @param n
 * 		number of bytes to search for the character
 * @return
 * 		non-zero if multibyte character encodings have state-dependent encodings,
 * 		otherwise zero
 */
int mbtowc(wchar_t* pwc, const char* s, size_t n);

/**
 * Converts the given wide character to a multi-byte character and stores the output
 * to <s> (if not NULL). Writes at most <MB_CUR_MAX> to the array. (N1548-7.22.7.3)
 *
 * @param s
 * 		output array
 * @param wc
 * 		wide character to convert
 * @return
 * 		non-zero if multibyte character encodings have state-dependent encodings,
 * 		otherwise zero
 */
int wctomb(char* s, wchar_t wc);

/**
 * Converts the multibyte character string <s> to a wide character string and writes
 * the result (of at most <n> elements) to <out>.
 *
 * @param out
 * 		output array
 * @param s
 * 		source string
 * @param n
 * 		maximum number of characters to write
 * @return
 * 		if successful, number of elements modified in the target array,
 * 		otherwise -1
 */
size_t mbstowcs(wchar_t* out, const char* s, size_t n);

/**
 * Converts the wide character string <s> to a multibyte character string and writes
 * the result (of at most <n> elements) to <out>.
 *
 * @param out
 * 		output array
 * @param s
 * 		source string
 * @param n
 * 		maximum number of bytes to write
 * @return
 * 		if successful, number of bytes modified in the target array,
 * 		otherwise -1
 */
size_t wcstombs(char* out, const wchar_t* s, size_t n);

/**
 * Creates a temporary filename, usign the <templ> as a part for templating.
 * The last six characters of this template must be 'X'.
 *
 * POSIX.1-2001
 *
 * @param templ
 * 		file name template to use
 *
 * @return the pointer to <templ>
 */
char* mktemp(char* templ);

// TODO
/**
 * POSIX, sets the environment variable <key>.
 */
int setenv(const char *key, const char *val, int overwrite);

__END_C

#endif
