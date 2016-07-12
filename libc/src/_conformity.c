/**
 * This header file is compiled with "-std=c11" only as a test to see if all
 * included headers are C11 conform.
 */

// freestanding GCC headers
#include <stdarg.h>
#include <stdatomic.h>
#include <stddef.h>

// libc headers
#include "assert.h"
#include "ctype.h"
#include "dlfcn.h"
#include "errno.h"
#include "inttypes.h"
#include "locale.h"
#include "malloc.h"
#include "setjmp.h"
#include "signal.h"
#include "stdalign.h"
#include "stdbool.h"
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "time.h"
#include "unistd.h"
#include "wchar.h"

// musl headers
#include "complex.h"
#include "endian.h"
#include "fenv.h"
#include "float.h"
#include "limits.h"
#include "math.h"

// ghost header
#include "ghost.h"

/**
 *
 */
void __g_libc_conformity_test() {
	return;
}
