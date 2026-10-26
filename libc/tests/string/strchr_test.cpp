#include <gtest/gtest.h>
#include "libc-under-test.hpp"

#include <cstddef>

TEST(string, strchr_finds_first_occurrence)
{
	const char text[] = "This is an example!";
	char* result = ghost_libc::strchr(text, 'a');

	ASSERT_NE(nullptr, result);
	EXPECT_EQ(static_cast<std::ptrdiff_t>(8), result - text);
	EXPECT_STREQ("an example!", result);
}

TEST(string, strchr_returns_null_when_missing)
{
	const char text[] = "This is an example!";
	EXPECT_EQ(nullptr, ghost_libc::strchr(text, 'z'));
}

TEST(string, strchr_can_find_string_terminator)
{
	const char text[] = "This is an example!";
	char* result = ghost_libc::strchr(text, '\0');

	ASSERT_NE(nullptr, result);
	EXPECT_EQ(static_cast<std::ptrdiff_t>(ghost_libc::strlen(text)), result - text);
}

TEST(string, strrchr_finds_last_occurrence)
{
	const char text[] = "bananas";
	char* result = ghost_libc::strrchr(text, 'a');

	ASSERT_NE(nullptr, result);
	EXPECT_EQ(static_cast<std::ptrdiff_t>(5), result - text);
	EXPECT_STREQ("as", result);
}

TEST(string, strrchr_returns_null_when_missing)
{
	const char text[] = "This is an example!";
	EXPECT_EQ(nullptr, ghost_libc::strrchr(text, 'z'));
}

TEST(string, strrchr_can_find_string_terminator)
{
	const char text[] = "This is an example!";
	char* result = ghost_libc::strrchr(text, '\0');

	ASSERT_NE(nullptr, result);
	EXPECT_EQ(static_cast<std::ptrdiff_t>(ghost_libc::strlen(text)), result - text);
}
