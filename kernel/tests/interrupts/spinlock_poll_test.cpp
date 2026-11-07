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

#include "support/fixture.hpp"

namespace
{
static int blockedLock;
static unsigned polls;
void pagingPollShootdown()
{
    ++polls;
    __atomic_store_n(&blockedLock, 0, __ATOMIC_RELEASE);
}

#include "production/spinlock_yield_hpp.inc"

TEST(SpinlockWait, ServicesPendingShootdownsBeforeAcquiring)
{
    blockedLock = 1;
    polls = 0;
    G_SPINLOCK_ACQUIRE(blockedLock);
    EXPECT_GT(polls, 0U);
    EXPECT_EQ(1, blockedLock);
    G_SPINLOCK_RELEASE(blockedLock);
    EXPECT_EQ(0, blockedLock);
}
}
