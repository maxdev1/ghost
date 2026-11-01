#include <gtest/gtest.h>
#include "libc-under-test.hpp"

#include <climits>

extern "C"
{
extern __thread int ghost_errno;
}

/**
 * These values follow Ghost's errno ABI, independently of the host's errno.h.
 */
TEST(string, strerror_describes_common_errors)
{
	const struct
	{
		int errnum;
		const char* message;
	} cases[] = {
		{0, "Success"},
		{2, "Permission denied"},
		{6, "Resource unavailable, try again"},
		{8, "Bad file descriptor"},
		{29, "Invalid argument"},
		{30, "I/O error"},
		{46, "No such file or directory"},
		{50, "Not enough memory"},
		{53, "No space left on device"},
		{62, "Operation not supported"},
		{65, "Operation not supported on socket"},
		{73, "Result too large"},
		{81, "Operation would block"},
		{82, "Cross-device link"}
	};

	for(const auto& entry : cases)
	{
		SCOPED_TRACE(entry.errnum);
		EXPECT_STREQ(entry.message, ghost_libc::strerror(entry.errnum));
	}
}

TEST(string, strerror_covers_every_defined_error)
{
	for(int errnum = 1; errnum <= 82; ++errnum)
	{
		if(errnum == 25 || errnum == 44 || errnum == 54 || errnum == 55 || errnum == 78)
		{
			continue;
		}

		SCOPED_TRACE(errnum);
		const char* message = ghost_libc::strerror(errnum);
		ASSERT_NE(nullptr, message);
		EXPECT_NE('\0', message[0]);
		EXPECT_STRNE("Unknown error", message);
		EXPECT_STRNE("ERROR(TODO)", message);
	}
}

TEST(string, strerror_handles_unknown_and_extreme_values)
{
	for(int errnum : {INT_MIN, -1, 25, 44, 54, 55, 78, 83, INT_MAX})
	{
		SCOPED_TRACE(errnum);
		EXPECT_STREQ("Unknown error", ghost_libc::strerror(errnum));
	}
}

TEST(string, strerror_preserves_thread_local_errno)
{
	ghost_errno = 73;
	for(int errnum : {INT_MIN, -1, 0, 2, 25, 46, 82, INT_MAX})
	{
		SCOPED_TRACE(errnum);
		ghost_libc::strerror(errnum);
		EXPECT_EQ(73, ghost_errno);
	}
}
