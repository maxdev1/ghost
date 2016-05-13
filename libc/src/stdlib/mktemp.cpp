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

#include "stdlib.h"
#include "string.h"
#include "errno.h"

/**
 *
 */
char* mktemp(char* templ) {

	if (templ == NULL) {
		return templ;
	}

	size_t templlen = strlen(templ);

	if (templlen < 6) {
		templ[0] = 0;
		return templ;
	}

	for (int i = templlen - 6; i < templlen; i++) {
		if (templ[i] != 'X') {
			errno = EINVAL;
			templ[0] = 0;
			return templ;
		}

		templ[i] = 'a' + rand() % ('z' - 'a');
	}

	return templ;
}
