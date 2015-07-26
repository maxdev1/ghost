#include <iostream>

#include "../src/string/memset.cpp"
#include "../src/ctype/tolower.cpp"
#include "../src/ctype/toupper.cpp"

#define TEST(name) \
	std::cout << #name << ": "; \
	if(test_##name()) { \
		std::cout << "success" << std::endl; \
	} else { \
		std::cout << "fail" << std::endl; \
	}

bool test_memset();
bool test_tolower();
bool test_toupper();

/**
 *
 */
int main(int argc, char** argv) {
	TEST(memset);
	TEST(tolower);
	TEST(toupper);
}

/**
 *
 */
bool test_tolower() {

	for (int i = 0; i < 128; i++) {

		char c = (char) i;
		char o = tolower(c);

		if (c >= 'A' && c <= 'Z') {
			if (c != o - 32) {
				std::cout << "tolower: '" << c << "' failed, got: '" << o << "'"
						<< std::endl;
				return false;
			}

		} else if (c != o) {
			std::cout << "tolower: something else than A-Z was lowercased"
					<< std::endl;
			return false;
		}

	}

	return true;
}

/**
 *
 */
bool test_toupper() {

	for (int i = 0; i < 128; i++) {

		char c = (char) i;
		char o = toupper(c);

		if (c >= 'a' && c <= 'z') {
			if (c != o + 32) {
				std::cout << "toupper: '" << c << "' failed, got: '" << o << "'"
						<< std::endl;
				return false;
			}

		} else if (c != o) {
			std::cout << "toupper: something else than A-Z was uppercased"
					<< std::endl;
			return false;
		}

	}

	return true;
}

/**
 *
 */
bool test_memset() {
	uint8_t* arr = new uint8_t[512];
	memset(arr, 0, 512);

	bool allfine = true;
	for (uint32_t i = 0; i < 512; i++) {
		if (arr[i] != 0) {
			allfine = false;
			break;
		}
	}
	return allfine;
}
