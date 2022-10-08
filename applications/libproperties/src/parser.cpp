/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2022, Max Schlüssel <lokoxe@gmail.com>                     *
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

#include "libproperties/parser.hpp"
#include <fstream>
#include <sstream>

const char* whitespace = " \t\n\r\f\v";

void g_properties_parser::initialize(std::string _content)
{
    current = -1;
    position = 0;
    content = _content;

    next();
}

g_properties_parser::g_properties_parser(std::ifstream& t)
{

    std::stringstream buffer;
    buffer << t.rdbuf();
    initialize(buffer.str());
}

g_properties_parser::g_properties_parser(std::string _content)
{
    initialize(_content);
}

std::map<std::string, std::string> g_properties_parser::getProperties()
{

    std::map<std::string, std::string> properties;

    while(true)
    {
        std::string a;
        std::string b;
        if(propertyEntry(a, b))
        {
            properties.insert(std::make_pair(a, b));
        }
        else
        {
            break;
        }
    }

    return properties;
}

bool g_properties_parser::propertyEntry(std::string& k, std::string& v)
{

    // Key
    if(!key(k))
    {
        return false;
    }

    // = or :
    next();

    // Value
    if(!value(v))
    {
        return false;
    }

    return true;
}

bool g_properties_parser::key(std::string& out)
{
    out = "";
    while(current != -1 && current != '=' && current != ':')
    {
        out += current;
        next();
    }

    out.erase(out.find_last_not_of(whitespace) + 1);
    out.erase(0, out.find_first_not_of(whitespace));
    return !out.empty();
}

bool g_properties_parser::value(std::string& out)
{
    out = "";
    while(current != -1 && current != '\n')
    {
        out += current;
        next();
    }

    out.erase(out.find_last_not_of(whitespace) + 1);
    out.erase(0, out.find_first_not_of(whitespace));

    if(out.length() > 1 && out[0] == '"' && out[out.length() - 1] == '"')
    {
        out = out.substr(1, out.length() - 2);
    }

    return !out.empty();
}

void g_properties_parser::next()
{
    if(position >= content.length())
    {
        current = -1;
        return;
    }

    current = content[position++];
}
