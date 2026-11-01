#include <gtest/gtest.h>
#include <climits>
#include <cstdio>
#include <cstring>

static int fgetsTestFailureAfter = -1;
static bool fgetsTestReadFailed = false;

extern "C"
{
char* ghost_fgets(char* destination, int count, FILE* stream);

/**
 * fgets only passes FILE pointers to these primitives. Native streams exercise
 * the production line reader without depending on Ghost's private FILE layout.
 */
int ghost_fgetc(FILE* stream)
{
	if(fgetsTestFailureAfter == 0)
	{
		fgetsTestReadFailed = true;
		return EOF;
	}
	if(fgetsTestFailureAfter > 0)
		--fgetsTestFailureAfter;
	return fgetc(stream);
}

int ghost_ferror(FILE* stream)
{
	return fgetsTestReadFailed || ferror(stream);
}
}

class Fgets : public testing::Test
{
protected:
	FILE* stream = nullptr;

	void SetUp() override
	{
		stream = tmpfile();
		ASSERT_NE(nullptr, stream);
		fgetsTestFailureAfter = -1;
		fgetsTestReadFailed = false;
	}

	void TearDown() override { if(stream) fclose(stream); }

	void fgetsTestInput(const char* input, size_t length)
	{
		ASSERT_EQ(length, fwrite(input, 1, length, stream));
		rewind(stream);
	}
};

TEST_F(Fgets, StopsAtNewlineAndLeavesNextLineUnread)
{
	fgetsTestInput("INSTALL\nnext\n", 13);
	char result[32];
	ASSERT_EQ(result, ghost_fgets(result, sizeof(result), stream));
	EXPECT_STREQ("INSTALL\n", result);
	ASSERT_EQ(result, ghost_fgets(result, sizeof(result), stream));
	EXPECT_STREQ("next\n", result);
	EXPECT_EQ(nullptr, ghost_fgets(result, sizeof(result), stream));
}

TEST_F(Fgets, IncludesAnEmptyLineAndTerminatesAtCapacity)
{
	fgetsTestInput("\nabcdef\n", 8);
	char result[5]{'x', 'x', 'x', 'x', 'x'};
	ASSERT_EQ(result, ghost_fgets(result, sizeof(result), stream));
	EXPECT_STREQ("\n", result);
	ASSERT_EQ(result, ghost_fgets(result, sizeof(result), stream));
	EXPECT_STREQ("abcd", result);
	ASSERT_EQ(result, ghost_fgets(result, sizeof(result), stream));
	EXPECT_STREQ("ef\n", result);
}

TEST_F(Fgets, ReturnsPartialFinalLineAndPreservesBufferAtImmediateEof)
{
	fgetsTestInput("tail", 4);
	char result[16];
	ASSERT_EQ(result, ghost_fgets(result, sizeof(result), stream));
	EXPECT_STREQ("tail", result);
	strcpy(result, "unchanged");
	EXPECT_EQ(nullptr, ghost_fgets(result, sizeof(result), stream));
	EXPECT_STREQ("unchanged", result);
}

TEST_F(Fgets, HandlesSmallCountsWithoutConsumingInputOrOverrunning)
{
	fgetsTestInput("x", 1);
	char result[2]{'a', 'b'};
	for(int count : {INT_MIN, -1, 0})
	{
		EXPECT_EQ(nullptr, ghost_fgets(result, count, stream));
		EXPECT_EQ('a', result[0]);
	}
	EXPECT_EQ(result, ghost_fgets(result, 1, stream));
	EXPECT_EQ('\0', result[0]);
	EXPECT_EQ('b', result[1]);
	EXPECT_EQ('x', fgetc(stream));
}

TEST_F(Fgets, PreservesEmbeddedNulAndHighBytes)
{
	const char input[]{'a', '\0', char(0xff), '\n', 'z'};
	fgetsTestInput(input, sizeof(input));
	char result[8];
	ASSERT_EQ(result, ghost_fgets(result, sizeof(result), stream));
	EXPECT_EQ(0, memcmp(input, result, 4));
	EXPECT_EQ('\0', result[4]);
	EXPECT_EQ('z', fgetc(stream));
}

TEST_F(Fgets, ReturnsNullOnReadErrorBeforeOrAfterInput)
{
	fgetsTestInput("input", 5);
	char result[16];
	fgetsTestFailureAfter = 0;
	EXPECT_EQ(nullptr, ghost_fgets(result, sizeof(result), stream));
	fgetsTestFailureAfter = 2;
	fgetsTestReadFailed = false;
	EXPECT_EQ(nullptr, ghost_fgets(result, sizeof(result), stream));
	EXPECT_TRUE(fgetsTestReadFailed);
}
