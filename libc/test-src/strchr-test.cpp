#include <iostream>

#include "../src/string/strchr.cpp"
#include "../src/string/strrchr.cpp"

/**
 *
 */
void test_strchr() {

	std::cout << strchr("This is an example!", 'a') << std::endl;

	char* x = strrchr("This is an example!", 'z');
	if (x != NULL) {
		std::cout << "failed! 'z' should not be found" << std::endl;
	}

	char* y = strrchr("This is an example!", 0);
	if (y == NULL) {
		std::cout << "failed! 0 must be found" << std::endl;
	}
}

/**
 *
 */
void test_strrchr() {

	std::cout << strrchr("This is an example!", 'a') << std::endl;

	char* x = strrchr("This is an example!", 'z');
	if (x != NULL) {
		std::cout << "failed! 'z' should not be found" << std::endl;
	}

	char* y = strrchr("This is an example!", 0);
	if (y == NULL) {
		std::cout << "failed! 0 must be found" << std::endl;
	}
}

/**
 *
 */
int main(int argc, char** argv) {
	test_strchr();
	test_strrchr();
}
