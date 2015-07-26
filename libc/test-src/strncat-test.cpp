#include <iostream>

#include "../src/string/strncat.cpp"
#include "../src/string/strcpy.cpp"

/**
 *
 */
void test_strncat() {

	char buf[128];
	const char* a = "Hello ";
	const char* b = "world!!!";
	strcpy(buf, a);
	strncat(buf, b, 7);
	std::cout << buf << std::endl;
}

/**
 *
 */
int main(int argc, char** argv) {
	test_strncat();
}
