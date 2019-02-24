#ifndef __TEST__
#define __TEST__

#include "test/test_logger.hpp"

int main();
void testAdd(const char* name, bool (*func)());

#define TEST(name)			\
bool name();				\
class name##TestAdder {		\
public:						\
	name##TestAdder() {		\
		testAdd(#name, name);\
	}						\
};							\
name##TestAdder name##Adder;\
bool name()

#define TOKENPASTE(x, y) x ## y
#define TOKENPASTE2(x, y) TOKENPASTE(x, y)
#define MOCK(name) TOKENPASTE2(name, __COUNTER__)

#define ASSERT_EQUALS(a, b)		if(!assertEquals(a, b)) return;

bool assertEquals(int a, int b);

#endif
