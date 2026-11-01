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

#ifndef NAVIGATOR_WORK_QUEUE_HPP
#define NAVIGATOR_WORK_QUEUE_HPP

#include <deque>
#include <functional>

namespace navigator
{
	/**
	 * Access is serialized by the service. Only adjacent replaceable requests from
	 * the same window may coalesce; navigation and close requests preserve order.
	 */
	class WorkQueue
	{
		struct Work
		{
			std::function<void()> run;
			const void* owner;
			int key;
		};
		std::deque<Work> pending;

	public:
		void queuePush(std::function<void()> work, const void* owner, int key)
		{
			if(owner && key >= 0 && !pending.empty() && pending.back().owner == owner && pending.back().key == key)
				pending.back().run = std::move(work);
			else
				pending.push_back({std::move(work), owner, key});
		}

		std::function<void()> queuePop()
		{
			if(pending.empty())
				return {};
			auto work = std::move(pending.front().run);
			pending.pop_front();
			return work;
		}
	};
} // namespace navigator
#endif
