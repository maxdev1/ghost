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

#include <ghost.h>
#include <atomic>
#include <cerrno>
#include <cstdio>
#include <cstdint>

extern "C" void* tlsTestLibraryFill(uint8_t value);
extern "C" bool tlsTestLibraryCheck(uint8_t value);
extern "C" __thread uint32_t tlsTestExported;

static thread_local uintptr_t localValue = 0x12345678;
static std::atomic<unsigned> ready{0};

struct g_tls_result
{
	uintptr_t thread;
	void* cache;
};

static g_tls_result results[4];

static void tlsTestRequire(bool success, const char* message)
{
	if(success)
		return;
	klog("tls-test: FAIL %s", message);
	g_exit(1);
}

static void tlsTestWorker(void* argument)
{
	auto index = reinterpret_cast<uintptr_t>(argument);
	auto value = uint8_t(index + 1);
	auto thread = reinterpret_cast<uintptr_t>(g_task_get_tls());
	tlsTestRequire(localValue == 0x12345678 && tlsTestExported == 29, "initial TLS values");
	localValue = value;
	errno = value;
	results[index] = {thread, tlsTestLibraryFill(value)};
	ready.fetch_add(1);

	while(ready.load() != 4)
		g_sleep(1);

	for(unsigned iteration = 0; iteration < 128; ++iteration)
	{
		uintptr_t self;
		asm volatile("movq %%fs:0, %0" : "=r"(self));
		tlsTestRequire(self == thread && *reinterpret_cast<uintptr_t*>(thread) == thread,
			"thread self pointer survived shared-library TLS writes");
		tlsTestRequire(localValue == value && errno == value && tlsTestExported == uint32_t(value + 29),
			"executable and named library TLS stay separate");
		tlsTestRequire(tlsTestLibraryCheck(value), "library cache stays private to each thread");

		g_sleep(1);
	}
}

int main()
{
	g_tid threads[4];
	for(uintptr_t i = 0; i < 4; ++i)
	{
		threads[i] = g_create_task_d(reinterpret_cast<void*>(tlsTestWorker), reinterpret_cast<void*>(i));
		tlsTestRequire(threads[i] != G_TID_NONE, "create TLS worker");
	}
	for(auto thread : threads)
		g_join(thread);
	for(unsigned i = 0; i < 4; ++i)
		for(unsigned j = 0; j < i; ++j)
			tlsTestRequire(results[i].thread != results[j].thread && results[i].cache != results[j].cache,
				"distinct thread storage");
	klog("tls-test: PASS");
	return 0;
}
