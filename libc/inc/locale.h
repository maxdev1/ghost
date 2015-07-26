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

#ifndef __GHOST_LIBC_LOCALE__
#define __GHOST_LIBC_LOCALE__

#include "ghost/common.h"
#include <stddef.h>

__BEGIN_C

// (N1548-7.11-2)
struct lconv {
	char *decimal_point; // "."
	char *thousands_sep; // ""
	char *grouping; // ""
	char *mon_decimal_point; // ""
	char *mon_thousands_sep; // ""
	char *mon_grouping; // ""
	char *positive_sign; // ""
	char *negative_sign; // ""
	char *currency_symbol; // ""
	char frac_digits; // CHAR_MAX
	char p_cs_precedes; // CHAR_MAX
	char n_cs_precedes; // CHAR_MAX
	char p_sep_by_space; // CHAR_MAX
	char n_sep_by_space; // CHAR_MAX
	char p_sign_posn; // CHAR_MAX
	char n_sign_posn; // CHAR_MAX
	char *int_curr_symbol; // ""
	char int_frac_digits; // CHAR_MAX
	char int_p_cs_precedes; // CHAR_MAX
	char int_n_cs_precedes; // CHAR_MAX
	char int_p_sep_by_space; // CHAR_MAX
	char int_n_sep_by_space; // CHAR_MAX
	char int_p_sign_posn; // CHAR_MAX
	char int_n_sign_posn; // CHAR_MAX
};

// (N1548-7.11-3)
#define LC_ALL			0
#define LC_COLLATE		1
#define LC_CTYPE		2
#define LC_MONETARY		3
#define LC_NUMERIC		4
#define LC_TIME			5

/**
 * Sets the program locale. (N1548-7.11.1.1)
 *
 * @param category
 * 		affected category
 * @param locale
 * 		locale to apply
 * @return
 * 		a pointer to the string associated with the specified
 * 		<category> if successful, otherwise a null pointer
 */
char* setlocale(int category, const char* locale);

/**
 * (N1548-7.11.2.1)
 */
struct lconv* localeconv();

__END_C

#endif
