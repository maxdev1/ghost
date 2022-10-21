/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2015, Max SchlÃƒÂ¼ssel <lokoxe@gmail.com>                     *
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

#include "js.hpp"

#include "console/duk_console.h"
#include "duktape.h"

#include <iostream>
#include <libterminal/terminal.hpp>

/**
 *
 */
inline bool ends_with(std::string const& value, std::string const& ending)
{
	if(ending.size() > value.size())
		return false;
	return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

/**
 *
 */
int main(int argc, char* argv[])
{
	duk_context* ctx = duk_create_heap_default();
	duk_console_init(ctx, DUK_CONSOLE_PROXY_WRAPPER /*flags*/);

	if(argc == 2)
	{
		auto buf = ftostr(argv[1]);
		duk_peval_string(ctx, (char*) buf);

		if(duk_get_type(ctx, -1) > DUK_TYPE_UNDEFINED)
		{
			printf("%s\n", duk_json_encode(ctx, -1));
		}

		return 0;
	}

	std::cout << "JS console (Duktape v" << (DUK_VERSION / 10000)
			  << "." << ((DUK_VERSION % 10000) / 100) << "."
			  << (DUK_VERSION % 100) << ")" << std::endl;
	while(true)
	{
		std::cout << "> ";
		std::flush(std::cout);
		std::string exec;
		std::getline(std::cin, exec);

		if(!ends_with(exec, ";"))
		{
			while(true)
			{
				std::cout << "~ ";
				std::string content;
				std::getline(std::cin, content);
				exec += content;
				if(content == "")
				{
					break;
				}
			}
		}

		if(duk_peval_string(ctx, exec.c_str()))
		{
			std::cout << (char) 27 << "[31m";
			std::cout << duk_safe_to_string(ctx, -1) << std::endl;
			std::cout << (char) 27 << "[0m";
		}
		else if(duk_get_type(ctx, -1) > DUK_TYPE_UNDEFINED)
		{
			printf("%s\n", duk_json_encode(ctx, -1));
		}
	}
	duk_destroy_heap(ctx);

	return 0;
}
