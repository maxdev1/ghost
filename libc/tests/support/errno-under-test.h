#ifndef GHOST_LIBC_TEST_ERRNO_UNDER_TEST_H
#define GHOST_LIBC_TEST_ERRNO_UNDER_TEST_H

#include <errno.h>

/**
 * Apply the test symbol name after Ghost's errno.h resets the errno macro.
 */
#undef errno
#define errno ghost_errno
extern __thread int errno;

#endif
