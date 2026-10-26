#include <gtest/gtest.h>
#include "libc-under-test.hpp"

#include <stdarg.h>

#include <string>
#include <utility>

namespace
{
ssize_t append_callback(void* param, const char* buf, size_t size)
{
	auto* output = static_cast<std::string*>(param);
	output->append(buf, size);
	return static_cast<ssize_t>(size);
}

std::pair<int, std::string> format(const char* pattern, ...)
{
	std::string output;

	va_list arglist;
	va_start(arglist, pattern);
	const int written = ghost_libc::vcbprintf(&output, append_callback, pattern, arglist);
	va_end(arglist);

	return {written, output};
}
}

TEST(stdio, vcbprintf_formats_signed_width_and_padding)
{
	const auto [written, output] = format("%+08i", 1337);

	EXPECT_EQ(std::string("+0001337"), output);
	EXPECT_EQ(static_cast<int>(output.size()), written);
}

TEST(stdio, vcbprintf_formats_radix_prefixes)
{
	const auto [written, output] = format("%#x %#o", 100, 100);

	EXPECT_EQ(std::string("0x64 0144"), output);
	EXPECT_EQ(static_cast<int>(output.size()), written);
}

TEST(stdio, vcbprintf_formats_variable_width_and_strings)
{
	const auto [written, output] = format("%*d %s", 5, 10, "Ghost");

	EXPECT_EQ(std::string("   10 Ghost"), output);
	EXPECT_EQ(static_cast<int>(output.size()), written);
}

TEST(stdio, vcbprintf_rejects_invalid_specifiers)
{
	const auto [written, output] = format("%q", 1);

	EXPECT_EQ(-1, written);
	EXPECT_EQ(std::string(""), output);
}
