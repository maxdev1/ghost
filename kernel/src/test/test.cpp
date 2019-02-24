#include <stdlib.h>
#include <stdio.h>
#include "test/test.hpp"

struct test_t
{
	bool (*test)();
	const char* name;
	test_t* next;
};

test_t* tests = 0;

void testAdd(const char* name, bool (*func)())
{
	test_t* n = (test_t*) malloc(sizeof(test_t));
	n->test = func;
	n->next = tests;
	n->name = name;
	tests = n;
}

int main()
{
	test_t* n = tests;
	while(n)
	{
		printf("%s:\n", n->name);
		if(!n->test())
		{
			printf("  Successful\n");
		} else
		{
			printf("  Failed\n");
		}
		n = n->next;
	}
}

bool assertEquals(int a, int b)
{
	if(a != b)
	{
		printf("Test failed! Expected %i but was %i\n", a, b);
		return false;
	}
	return true;
}
