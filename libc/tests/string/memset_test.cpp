#include <gtest/gtest.h>
#include "libc-under-test.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

TEST(string, memset_fills_entire_buffer)
{
	std::array<std::uint8_t, 64> buffer {};
	void* result = ghost_libc::memset(buffer.data(), 0xA5, buffer.size());

	EXPECT_EQ(static_cast<void*>(buffer.data()), result);
	for(std::size_t i = 0; i < buffer.size(); ++i)
	{
		EXPECT_EQ(0xA5, static_cast<int>(buffer[i]));
	}
}

TEST(string, memset_supports_zero_length)
{
	std::array<std::uint8_t, 4> buffer {1, 2, 3, 4};
	ghost_libc::memset(buffer.data(), 0xFF, 0);

	EXPECT_EQ(1, static_cast<int>(buffer[0]));
	EXPECT_EQ(2, static_cast<int>(buffer[1]));
	EXPECT_EQ(3, static_cast<int>(buffer[2]));
	EXPECT_EQ(4, static_cast<int>(buffer[3]));
}
