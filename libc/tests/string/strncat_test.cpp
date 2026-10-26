#include <gtest/gtest.h>
#include "libc-under-test.hpp"

TEST(string, strncat_appends_up_to_limit)
{
	char buffer[32];
	ghost_libc::strcpy(buffer, "Hello ");

	char* result = ghost_libc::strncat(buffer, "world!!!", 5);
	EXPECT_EQ(buffer, result);
	EXPECT_STREQ("Hello world", buffer);
}

TEST(string, strncat_appends_entire_string_when_limit_is_large)
{
	char buffer[32];
	ghost_libc::strcpy(buffer, "Ghost");

	ghost_libc::strncat(buffer, " OS", 16);
	EXPECT_STREQ("Ghost OS", buffer);
}

TEST(string, strncat_does_nothing_for_zero_length)
{
	char buffer[32];
	ghost_libc::strcpy(buffer, "Ghost");

	ghost_libc::strncat(buffer, " kernel", 0);
	EXPECT_STREQ("Ghost", buffer);
}
