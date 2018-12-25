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

#include "stdio.h"
#include "errno.h"
#include "wchar.h"
#include "string.h"

#define LENGTH_DEFAULT	0
#define LENGTH_hh		1
#define LENGTH_h		2
#define LENGTH_l		3
#define LENGTH_ll		4
#define LENGTH_j		5
#define LENGTH_z		6
#define LENGTH_t		7
#define LENGTH_L		8

#define STEP			if (!*++s) return written;

/**
 * Tries to get a simple integer number from the current string location;
 * returns 0 if there was no number.
 */
static int _get_number(const char** s) {

	int res = 0;
	const char* p = *s;

	while (*p >= '0' && *p <= '9') {
		res = (res * 10) + (*p - '0');
		++p;
	}

	*s = p;
	return res;
}

/**
 * Converts the given number to it's string representation
 */
static size_t _integer_to_string(char* out, uintmax_t value, uintmax_t base,
		const char* digits) {

	size_t count = 1;

	// check how many characters are required
	uintmax_t temp = value;
	while (temp >= base) {
		temp /= base;
		count++;
	}

	// convert to digits
	for (size_t i = count; i != 0; i--) {
		out[i - 1] = digits[value % base];
		value /= base;
	}

	return count;
}

/**
 *
 */
int vcbprintf(void* param,
		ssize_t (*callback)(void* param, const char* buf, size_t maximum),
		const char *format, va_list arglist) {

	const char* s = format;
	int written = 0;

	char number_buffer[24];
	char precision_buffer[24];

	// iterate through format characters
	while (*s) {

		// enter format
		if (*s == '%') {

			STEP;

			// early exit on '%'
			if (*s == '%') {
				if (callback(param, s, 1) != 1)
					return -1;
				++written;
				STEP;
				continue;
			}

			// flags
			int flag_left_justify = 0;
			int flag_always_prepend_sign = 0;
			int flag_always_prepend_space_plussign = 0;
			int flag_force_0x_or_dec = 0;
			int flag_left_pad_zeroes = 0;

			int flag_found;
			do {
				flag_found = 0;

				switch (*s) {
				case '-':
					flag_left_justify = 1;
					flag_found = 1;
					break;
				case '+':
					flag_always_prepend_sign = 1;
					flag_found = 1;
					break;
				case ' ':
					flag_always_prepend_space_plussign = 1;
					flag_found = 1;
					break;
				case '#':
					flag_force_0x_or_dec = 1;
					flag_found = 1;
					break;
				case '0':
					flag_left_pad_zeroes = 1;
					flag_found = 1;
					break;
				}

				if (flag_found) {
					STEP;
				}
			} while (flag_found);

			// width
			int width = 0;

			if (*s == '*') { // take from argument list
				width = va_arg(arglist, int);
				STEP;

			} else if (*s >= '0' && *s <= '9') { // take from format
				width = _get_number(&s);
				// -> already stepped by _get_number
			}

			// precision
			int precision = 6;
			int explicitPrecision = 0;

			if (*s == '.') {
				STEP;
				explicitPrecision = 1;

				if (*s == '*') { // take from argument list
					precision = va_arg(arglist, int);
					STEP;

				} else if (*s >= '0' && *s <= '9') { // take from format
					precision = _get_number(&s);
					// -> already stepped by _get_number
				}
			}

			// length
			int length = LENGTH_DEFAULT;

			if (*s == 'h') {
				STEP;

				if (*s == 'h') {
					length = LENGTH_hh;
					STEP;
				} else {
					length = LENGTH_h;
				}

			} else if (*s == 'l') {
				STEP;

				if (*s == 'l') {
					length = LENGTH_ll;
					STEP;
				} else {
					length = LENGTH_l;
				}

			} else if (*s == 'j') {
				STEP;
				length = LENGTH_j;

			} else if (*s == 'z') {
				STEP;
				length = LENGTH_z;

			} else if (*s == 't') {
				STEP;
				length = LENGTH_t;

			} else if (*s == 'L') {
				STEP;
				length = LENGTH_L;

			}

			// use specifier and length to get value of argument
			char specifier = *s;

			switch (*s) {

			case 'u':
			case 'o':
			case 'x':
			case 'X':
			case 'd':
			case 'i':
			case 'p': {
				uintmax_t value;
				int issigned;

				if (specifier == 'd' || specifier == 'i') {
					issigned = 1;

					switch (length) {
					case LENGTH_DEFAULT:
						value = va_arg(arglist, int);
						break;
					case LENGTH_hh:
						value = (signed char) va_arg(arglist, int);
						break;
					case LENGTH_h:
						value = (short int) va_arg(arglist, int);
						break;
					case LENGTH_l:
						value = va_arg(arglist, long int);
						break;
					case LENGTH_ll:
						value = va_arg(arglist, long long int);
						break;
					case LENGTH_j:
						value = va_arg(arglist, intmax_t);
						break;
					case LENGTH_z:
						value = va_arg(arglist, size_t);
						break;
					case LENGTH_t:
						value = va_arg(arglist, ptrdiff_t);
						break;
					default:
						errno = EINVAL;
						return -1;
					}

				} else {
					issigned = 0;

					switch (length) {
					case LENGTH_DEFAULT:
						value = va_arg(arglist, unsigned int);
						break;
					case LENGTH_hh:
						value = (unsigned char) va_arg(arglist, int);
						break;
					case LENGTH_h:
						value = (unsigned short int) va_arg(arglist, int);
						break;
					case LENGTH_l:
						value = va_arg(arglist, unsigned long int);
						break;
					case LENGTH_ll:
						value = va_arg(arglist, unsigned long long int);
						break;
					case LENGTH_j:
						value = va_arg(arglist, uintmax_t);
						break;
					case LENGTH_z:
						value = va_arg(arglist, size_t);
						break;
					case LENGTH_t:
						value = va_arg(arglist, ptrdiff_t);
						break;
					default:
						errno = EINVAL;
						return -1;
					}
				}

				// write number in temporary buffer
				const char* digits;
				int base;
				int isnegative = (issigned && ((intmax_t) value) < 0);

				// pointers are printed as numbers
				if (specifier == 'p') {
					specifier = 'x';
					flag_force_0x_or_dec = 1;
				}

				// decide for base and digits
				if (specifier == 'x') {
					base = 16;
					digits = "0123456789abcdef";
				} else if (specifier == 'X') {
					base = 16;
					digits = "0123456789ABCDEF";
				} else if (specifier == 'o') {
					base = 8;
					digits = "012345678";
				} else {
					base = 10;
					digits = "0123456789";
				}

				ssize_t len = _integer_to_string(number_buffer,
						(isnegative ? -((intmax_t) value) : value), base,
						digits);

				// check for additionals
				int additionalslen = 0;

				if (flag_always_prepend_sign
						|| flag_always_prepend_space_plussign || isnegative) {
					++additionalslen; // +/-/space
				}

				if (flag_force_0x_or_dec
						&& (specifier == 'x' || specifier == 'X')) {
					additionalslen += 2; // 0x/0X
				}

				if (flag_force_0x_or_dec && specifier == 'o') {
					++additionalslen; // 0
				}

				// adjust width to pad correctly
				if (width < len + additionalslen) {
					width = len + additionalslen;
				}

				// left padding with spaces
				if (!flag_left_justify && !flag_left_pad_zeroes) {
					for (int i = 0; i < width - len - additionalslen; i++) {
						if (callback(param, " ", 1) != 1)
							return -1;
						++written;
					}
				}

				// sign
				if (isnegative) {
					if (callback(param, "-", 1) != 1)
						return -1;
					++written;

				} else if (flag_always_prepend_sign) {
					if (callback(param, "+", 1) != 1)
						return -1;
					++written;

				} else if (flag_always_prepend_space_plussign) {
					if (callback(param, " ", 1) != 1)
						return -1;
					++written;
				}

				// left padding with zeroes
				if (!flag_left_justify && flag_left_pad_zeroes) {
					for (int i = 0; i < width - len - additionalslen; i++) {
						if (callback(param, "0", 1) != 1)
							return -1;
						++written;
					}
				}

				// additionals
				if (flag_force_0x_or_dec
						&& (specifier == 'x' || specifier == 'X')) {
					if (callback(param, "0", 1) != 1)
						return -1;
					++written;

					if (specifier == 'x') {
						if (callback(param, "x", 1) != 1)
							return -1;
					} else {
						if (callback(param, "X", 1) != 1)
							return -1;
					}
					++written;
				} else if (flag_force_0x_or_dec && specifier == 'o') {
					if (callback(param, "0", 1) != 1)
						return -1;
					++written;
				}

				// write number
				if (callback(param, number_buffer, len) != len)
					return -1;
				written += len;

				// right padding
				if (flag_left_justify) {
					for (int i = 0; i < width - len - additionalslen; i++) {
						if (callback(param, " ", 1) != 1)
							return -1;
						++written;
					}
				}

				break;
			}

			case 'f':
			case 'F':
			case 'e':
			case 'E':
			case 'g':
			case 'G':
			case 'a':
			case 'A': {
				long double value;
				switch (length) {
				case LENGTH_DEFAULT:
					value = va_arg(arglist, double);
					break;
				case LENGTH_L:
					value = va_arg(arglist, long double);
					break;
				default:
					errno = EINVAL;
					return -1;
				}

				// TODO this implementation only does simple printing, printing
				// floating point numbers is much more complex, but okay for now

				// TODO scientific and hexadecimal numbers are converted as normal
				// decimals here. long doubles won't work properly as well.

				// store negativity and make value positive
				int isnegative = value < 0;
				if (isnegative) {
					value = -value;
				}

				// get before comma stuff
				uintmax_t value_integer = (uintmax_t) value;

				// get after comma stuff
				double value_fractional_d = value - ((intmax_t) value);
				while (value_fractional_d - ((intmax_t) value_fractional_d) != 0) {
					value_fractional_d *= 10;
				}
				uintmax_t value_fractional = (uintmax_t) value_fractional_d;

				// write number in temporary buffer
				size_t len_int = _integer_to_string(number_buffer,
						value_integer, 10, "0123456789");
				size_t len_fract = _integer_to_string(precision_buffer,
						value_fractional, 10, "0123456789");
				if (len_fract > precision) {
					len_fract = precision;
				}

				// check for additionals
				int additionalslen = 0;

				if (flag_always_prepend_sign
						|| flag_always_prepend_space_plussign || isnegative) {
					++additionalslen; // +/-/space
				}

				if (flag_force_0x_or_dec) {
					++additionalslen; // decimal dot
				} else {
					// check if the characters that will be printed
					// after the decimal dot will be other than zero,
					// otherwise it is neglected
					for (int i = 0; i < len_fract; i++) {
						if (precision_buffer[i] != '0') {
							++additionalslen; // we'll need a decimal dot
							break;
						}
					}
				}

				// adjust width to pad correctly
				if (width < len_int + additionalslen + precision) {
					width = len_int + additionalslen + precision;
				}

				// left padding with spaces
				if (!flag_left_justify && !flag_left_pad_zeroes) {
					for (int i = 0;
							i < width - len_int - additionalslen - precision;
							i++) {
						if (callback(param, " ", 1) != 1)
							return -1;
						++written;
					}
				}

				// sign
				if (isnegative) {
					if (callback(param, "-", 1) != 1)
						return -1;
					++written;

				} else if (flag_always_prepend_sign) {
					if (callback(param, "+", 1) != 1)
						return -1;
					++written;

				} else if (flag_always_prepend_space_plussign) {
					if (callback(param, " ", 1) != 1)
						return -1;
					++written;
				}

				// left padding with zeroes
				if (!flag_left_justify && !flag_left_pad_zeroes) {
					for (int i = 0;
							i < width - len_int - additionalslen - precision;
							i++) {
						if (callback(param, "0", 1) != 1)
							return -1;
						++written;
					}
				}

				// write integer part
				if (callback(param, number_buffer, len_int) != len_int)
					return -1;
				written += len_int;

				if (flag_force_0x_or_dec || value_fractional != 0) {
					if (callback(param, ".", 1) != 1)
						return -1;
					++written;

					// write fractional part
					if (callback(param, precision_buffer, len_fract)
							!= len_fract)
						return -1;
					written += len_fract;

					// write precision filling zeroes
					for (size_t i = 0;
							i < (len_fract == 0 ? 1 : precision - len_fract);
							i++) {
						if (callback(param, "0", 1) != 1)
							return -1;
						++written;
					}
				}

				// right padding
				if (flag_left_justify) {
					for (int i = 0;
							i < width - len_int - additionalslen - precision;
							i++) {
						if (callback(param, " ", 1) != 1)
							return -1;
						++written;
					}
				}

				break;
			}

			case 'c': {
				char value;
				switch (length) {
				case LENGTH_DEFAULT:
					value = va_arg(arglist, int);
					break;
				case LENGTH_l:
					value = va_arg(arglist, wint_t);
					break;
				default:
					errno = EINVAL;
					return -1;
				}

				// left padding
				if (!flag_left_justify) {
					for (int i = 0; i < width - 1; i++) {
						if (callback(param, " ", 1) != 1)
							return -1;
						++written;
					}
				}

				// print character TODO wchar_t?
				if (callback(param, &value, 1) != 1)
					return -1;
				++written;

				// right padding
				if (flag_left_justify) {
					for (int i = 0; i < width - 1; i++) {
						if (callback(param, " ", 1) != 1)
							return -1;
						++written;
					}
				}

				break;
			}

			case 's': {
				char* value;
				switch (length) {
				case LENGTH_DEFAULT:
					value = (char*) va_arg(arglist, char*);
					break;
				case LENGTH_l:
					value = (char*) va_arg(arglist, wchar_t*);
					break;
				default:
					errno = EINVAL;
					return -1;
				}

				// get length
				size_t len = strlen(value);

				if (width < len) {
					width = len;
				}

				// left padding
				if (!flag_left_justify) {
					for (int i = 0; i < width - len; i++) {
						if (callback(param, " ", 1) != 1)
							return -1;
						++written;
					}
				}
				
				// limit the output if a precision was explicitly set
				if (explicitPrecision && len > precision) {
					len = precision;
				}

				// write string TODO wchar_t?
				if (callback(param, value, len) != len)
					return -1;
				written += len;

				// expand output if a precision was explicitly set
				if (explicitPrecision && len < precision) {
					for (int i = 0; i < precision - len; i++) {
						if (callback(param, " ", 1) != 1)
							return -1;
						++written;
					}
				}

				// right padding
				if (flag_left_justify) {
					for (int i = 0; i < width - len; i++) {
						if (callback(param, " ", 1) != 1)
							return -1;
						++written;
					}
				}

				break;
			}

			case 'n': {
				signed int* value = (signed int*) va_arg(arglist, void*);
				*value = written;
				break;
			}

			default: {
				errno = EINVAL;
				return -1;
			}
			}

		} else {
			if (callback(param, s, 1) != 1)
				return -1;
			++written;
		}

		STEP;
	}

	return written;
}
