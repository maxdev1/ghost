#include <gtest/gtest.h>
#include "libc-under-test.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace
{
int compare_ints(const void* left, const void* right)
{
	const auto lhs = *static_cast<const int*>(left);
	const auto rhs = *static_cast<const int*>(right);

	if(lhs < rhs)
	{
		return -1;
	}
	if(lhs > rhs)
	{
		return 1;
	}
	return 0;
}

struct sortable_t
{
	int key;
	int marker;
	std::uint8_t payload[256];
	int tail;
};

int compare_structs(const void* left, const void* right)
{
	const auto* lhs = static_cast<const sortable_t*>(left);
	const auto* rhs = static_cast<const sortable_t*>(right);

	if(lhs->key < rhs->key)
	{
		return -1;
	}
	if(lhs->key > rhs->key)
	{
		return 1;
	}
	return 0;
}
}

TEST(stdlib, qsort_sorts_integers)
{
	std::array<int, 8> values {7, 3, 9, 1, 4, 1, 5, 2};
	const std::array<int, 8> expected {1, 1, 2, 3, 4, 5, 7, 9};

	ghost_libc::qsort(values.data(), values.size(), sizeof(values[0]), compare_ints);

	for(std::size_t i = 0; i < values.size(); ++i)
	{
		EXPECT_EQ(expected[i], values[i]);
	}
}

TEST(stdlib, qsort_handles_empty_ranges)
{
	int value = 42;
	ghost_libc::qsort(&value, 0, sizeof(value), compare_ints);
	EXPECT_EQ(42, value);
}

TEST(stdlib, qsort_swaps_full_struct_payloads)
{
	std::array<sortable_t, 5> values {};
	const std::array<int, 5> keys {5, 1, 4, 2, 3};

	for(std::size_t i = 0; i < values.size(); ++i)
	{
		values[i].key = keys[i];
		values[i].marker = static_cast<int>(0x10 + i);
		values[i].tail = static_cast<int>(0x20 + i);
		for(std::size_t j = 0; j < sizeof(values[i].payload); ++j)
		{
			values[i].payload[j] = static_cast<std::uint8_t>(values[i].marker);
		}
	}

	ghost_libc::qsort(values.data(), values.size(), sizeof(values[0]), compare_structs);

	for(std::size_t i = 1; i < values.size(); ++i)
	{
		EXPECT_TRUE(values[i - 1].key <= values[i].key);
	}

	for(const auto& value : values)
	{
		EXPECT_EQ(value.marker + 0x10, value.tail);
		for(std::size_t j = 0; j < sizeof(value.payload); ++j)
		{
			EXPECT_EQ(value.marker, static_cast<int>(value.payload[j]));
		}
	}
}
