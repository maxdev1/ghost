#ifndef GHOST_LIBC_TEST_UNDER_TEST_HPP
#define GHOST_LIBC_TEST_UNDER_TEST_HPP

#include <stdarg.h>
#include <stddef.h>
#include <sys/types.h>

extern "C"
{
void* ghost_memset(void* mem, int value, size_t len);
int ghost_tolower(int c);
int ghost_toupper(int c);
void ghost_qsort(void* array, size_t elements, size_t size, int (*comparator)(const void*, const void*));
char* ghost_strchr(const char* str, int value);
char* ghost_strrchr(const char* str, int value);
char* ghost_strncat(char* dest, const char* src, size_t max);
char* ghost_strcpy(char* dest, const char* src);
size_t ghost_strlen(const char* str);
int ghost_vcbprintf(void* param,
                    ssize_t (*callback)(void* param, const char* buf, size_t maximum),
                    const char* format, va_list arglist);
}

namespace ghost_libc
{
using qsort_comparator_t = int (*)(const void*, const void*);
using vcbprintf_callback_t = ssize_t (*)(void* param, const char* buf, size_t maximum);

inline void* memset(void* mem, int value, size_t len)
{
	return ::ghost_memset(mem, value, len);
}

inline int tolower(int c)
{
	return ::ghost_tolower(c);
}

inline int toupper(int c)
{
	return ::ghost_toupper(c);
}

inline void qsort(void* array, size_t elements, size_t size, qsort_comparator_t comparator)
{
	::ghost_qsort(array, elements, size, comparator);
}

inline char* strchr(const char* str, int value)
{
	return ::ghost_strchr(str, value);
}

inline char* strrchr(const char* str, int value)
{
	return ::ghost_strrchr(str, value);
}

inline char* strncat(char* dest, const char* src, size_t max)
{
	return ::ghost_strncat(dest, src, max);
}

inline char* strcpy(char* dest, const char* src)
{
	return ::ghost_strcpy(dest, src);
}

inline size_t strlen(const char* str)
{
	return ::ghost_strlen(str);
}

inline int vcbprintf(void* param, vcbprintf_callback_t callback, const char* format, va_list arglist)
{
	return ::ghost_vcbprintf(param, callback, format, arglist);
}
}

#endif
