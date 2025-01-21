#include "test/test.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int main(int argc, const char** argv)
{
	const char* only = nullptr;
	if(argc == 2)
	{
		only = argv[1];
	}

	for(test_t* test = tests; test; test = test->next)
	{
		if(only != nullptr && strstr(test->name, only) != test->name)
			continue;

		printf("%s", test->name);
		try
		{
			test->test();
			printf(" \e[1;92m✓\e[0m\n");
		}
		catch(const char* e)
		{
			printf(" \e[1;31m❌\e[0m\n");
			printf(" %s\n", e);
		}
	}
}

void _panic(int line, const char* msg, ...)
{
	char buf1[BUFLEN];
	va_list valist;
	va_start(valist, msg);
	vsnprintf(buf1, BUFLEN, msg, valist);
	va_end(valist);

	char buf2[BUFLEN];
	snprintf(buf2, BUFLEN, "\t%i: panic: %s", line, buf1);
	throw buf2;
}
