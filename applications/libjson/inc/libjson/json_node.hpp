/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2025, Max Schlüssel <lokoxe@gmail.com>                     *
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

#ifndef LIBJSON_JSON_NODE
#define LIBJSON_JSON_NODE

#include <string>
#include <variant>
#include <vector>
#include <map>

struct g_json_node
{
    using object = std::map<std::string, g_json_node>;
    using array = std::vector<g_json_node>;
    using value = std::variant<std::nullptr_t, bool, double, std::string, array, object>;

    value v{};

    g_json_node() : v(nullptr)
    {
    }

    g_json_node(std::nullptr_t) : v(nullptr)
    {
    }

    g_json_node(bool b) : v(b)
    {
    }

    g_json_node(double d) : v(d)
    {
    }

    g_json_node(const char* s) : v(std::string(s))
    {
    }

    g_json_node(std::string s) : v(std::move(s))
    {
    }

    g_json_node(array a) : v(std::move(a))
    {
    }

    g_json_node(object o) : v(std::move(o))
    {
    }

    bool isObject() const { return std::holds_alternative<object>(v); }
    bool isArray() const { return std::holds_alternative<array>(v); }
    bool isString() const { return std::holds_alternative<std::string>(v); }
    bool isNumber() const { return std::holds_alternative<double>(v); }
    bool isBool() const { return std::holds_alternative<bool>(v); }
    bool isNull() const { return std::holds_alternative<std::nullptr_t>(v); }

    object& asObject() { return std::get<object>(v); }
    array& asArray() { return std::get<array>(v); }
    std::string& asString() { return std::get<std::string>(v); }
    double& asNumber() { return std::get<double>(v); }
    bool& asBool() { return std::get<bool>(v); }
};

#endif
