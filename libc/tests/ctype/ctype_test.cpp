#include <gtest/gtest.h>
#include "libc-under-test.hpp"

TEST(ctype, tolower_converts_ascii_uppercase)
{
	for(int code = 'A'; code <= 'Z'; ++code)
	{
		EXPECT_EQ(code - 'A' + 'a', ghost_libc::tolower(code));
	}
}

TEST(ctype, tolower_leaves_other_ascii_values_unchanged)
{
	for(int code = 0; code < 128; ++code)
	{
		if(code >= 'A' && code <= 'Z')
		{
			continue;
		}

		EXPECT_EQ(code, ghost_libc::tolower(code));
	}
}

TEST(ctype, toupper_converts_ascii_lowercase)
{
	for(int code = 'a'; code <= 'z'; ++code)
	{
		EXPECT_EQ(code - 'a' + 'A', ghost_libc::toupper(code));
	}
}

TEST(ctype, toupper_leaves_other_ascii_values_unchanged)
{
	for(int code = 0; code < 128; ++code)
	{
		if(code >= 'a' && code <= 'z')
		{
			continue;
		}

		EXPECT_EQ(code, ghost_libc::toupper(code));
	}
}
