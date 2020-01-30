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

#include <stdio.h>
#include <ghost.h>

#define ASSERT(expression)	\
	if(!(expression)) {		\
		klog("[Test failed] %s, line %i: " #expression, __FUNCTION__, __LINE__);	\
		return test_result_t(0, 1);													\
	}

#define TEST_SUCCESSFUL		\
	klog("[Test successful] %s, line %i", __FUNCTION__, __LINE__); \
	return test_result_t(1, 0);

#define TEST_FAILED			\
	klog("[Test failed] %s, line %i", __FUNCTION__, __LINE__); \
	return test_result_t(0, 1);


class test_result_t {
public:
	int successful;
	int failed;
	test_result_t() : successful(0), failed(0) {}
	test_result_t(int successful, int failed) : successful(successful), failed(failed) {}

	friend test_result_t operator+(const test_result_t& a, const test_result_t& b) {
		test_result_t out;
		out.successful = a.successful + b.successful;
		out.failed = a.failed + b.failed;
		return out;
	}

	test_result_t& operator+=(const test_result_t& other) {
		this->successful += other.successful;
		this->failed += other.failed;
		return *this;
	}
};

test_result_t runMessageTest();

test_result_t runStdioTest();

test_result_t runThreadTests();
