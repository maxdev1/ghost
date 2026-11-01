/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2026, Max Schlüssel <lokoxe@gmail.com>                     *
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

#ifndef NAVIGATOR_INTERFACE_HPP
#define NAVIGATOR_INTERFACE_HPP

#include <cstdint>
#include <cstring>
#include <map>
#include <string>

namespace navigator
{
	constexpr const char* serviceName = "navigator";
	constexpr uint32_t openMagic = 0x4e415631;
	constexpr size_t pathLimit = 4096, fragmentSize = 1024;

	struct OpenRequest
	{
		uint32_t magic = openMagic;
		uint16_t total = 0, offset = 0, length = 0;
		char path[fragmentSize]{};
	};

	enum class OpenStatus : uint32_t
	{
		Invalid,
		Continue,
		Accepted,
		Busy
	};

	/**
	 * Paths can exceed one Ghost IPC packet. Fragments are acknowledged separately
	 * and assembled per sender/transaction before a window request is accepted.
	 */
	class OpenRequests
	{
		struct Pending
		{
			int transaction;
			size_t total;
			std::string path;
		};
		std::map<int, Pending> pending;

	public:
		OpenStatus interfaceReceive(int sender, int transaction, const OpenRequest& request, std::string& path)
		{
			path.clear();
			if(request.magic != openMagic || request.total == 0 || request.total >= pathLimit || request.length == 0 ||
			   request.length > fragmentSize || size_t(request.offset) + request.length > request.total ||
			   std::memchr(request.path, '\0', request.length))
			{
				pending.erase(sender);
				return OpenStatus::Invalid;
			}
			if(request.offset == 0)
			{
				if(!pending.count(sender) && pending.size() >= 32)
					return OpenStatus::Busy;
				pending[sender] = {transaction, request.total, {}};
			}
			auto found = pending.find(sender);
			if(found == pending.end())
				return OpenStatus::Invalid;
			auto& part = found->second;
			if(part.transaction != transaction || part.total != request.total || part.path.size() != request.offset)
			{
				pending.erase(found);
				return OpenStatus::Invalid;
			}
			part.path.append(request.path, request.length);
			if(part.path.size() < part.total)
				return OpenStatus::Continue;
			path = std::move(part.path);
			pending.erase(found);
			return path.front() == '/' ? OpenStatus::Accepted : OpenStatus::Invalid;
		}
	};
} // namespace navigator
#endif
