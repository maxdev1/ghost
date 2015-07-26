#include <iostream>

// include source files (can't be linked)
//#include "../src/string/memset.cpp"
typedef void (*vcbprintf_callback_t)(char c, void **param);
#include "../src/stdio/vcbprintf.cpp"

bool test_vcbprintf();

/**
 *
 */
int main(int argc, char** argv) {
	test_vcbprintf();
}

/**
 *
 */
size_t test_vcbprintf_callback(void* param, const char* buf, size_t size) {
	for (size_t i = 0; i < size; i++) {
		std::cout << buf[i];
	}
	return size;
}

/**
 *
 */
void test_printf(const char *format, ...) {

	va_list va;
	va_start(va, format);
	vcbprintf(0, test_vcbprintf_callback, format, va);
	va_end(va);
}

/**
 *
 */
bool test_vcbprintf() {

	test_printf("%+08i\n", 1337);
	int x;
	test_printf("%llp\n", &x);
	std::cout << std::hex << &x << std::endl;
	test_printf("Characters: %c %c \n", 'a', 65);
	test_printf("Decimals: %d %ld\n", 1977, 650000L);
	test_printf("Preceding with blanks: %10d \n", 1977);
	test_printf("Preceding with zeros: %010d \n", 1977);
	test_printf("Some different radices: %d %x %o %#x %#o \n", 100, 100, 100,
			100, 100);
	test_printf("floats: %4.2f %+.0e %E %a \n", 3.1416, 3.1416, 3.1416, -5.42);
	test_printf("Width trick: %*d \n", 5, 10);
	test_printf("%s \n", "A string");
	return true;
}
