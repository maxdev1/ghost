#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"

static uint8_t heap_lock = 0;
static uint8_t* heap_start = 0;
static uint8_t* heap_used = 0;
static uint8_t* heap_end = 0;

#define MAGIC 0xDEADBEEF
#define OUTPUT_FAILURE(addr, msg, previous_free)	\
		void ** puEBP = NULL;	\
		__asm__ __volatile__ ("mov %%ebp, %0" : "=r"(puEBP));	\
		outputfailure(addr, msg, previous_free, puEBP)

/**
 *
 */
typedef struct {
	size_t size;
	int times_freed;
	uint32_t previous_free;
	uint32_t magic;
} allocation_header_t __attribute__((packed));

void outputfailure(void* addr, const char* message, uint32_t previous_free, void** stackframe) {
	klog("free(%x) %s: (prevfree: %x) ebp %x caller %x", addr, message, previous_free, stackframe[0], stackframe[1]);
}

/**
 *
 */
void free(void* ptr) {

	if(ptr == 0) {
		return;
	}

	g_atomic_lock(&heap_lock);

	allocation_header_t* header = (allocation_header_t*) ((uint8_t*) ptr - sizeof(allocation_header_t));
	if(header->magic != MAGIC) {
		OUTPUT_FAILURE(ptr, "illegal magic", 0);
		for(;;);
	}

	header->times_freed++;
	if(header->times_freed > 1) {
		OUTPUT_FAILURE(ptr, "multi-free", header->previous_free);
		for(;;);
	}

	void** ebp = NULL;
	__asm__ __volatile__ ("mov %%ebp, %0" : "=r"(ebp));
	header->previous_free = (uint32_t) ebp[1];

	heap_lock = 0;
}

/**
 *
 */
void* malloc(size_t size) {

	g_atomic_lock(&heap_lock);

	if (heap_start == 0) {
		heap_start = (uint8_t*) sbrk(0);
		heap_end = heap_start;
		heap_used = heap_start;
	}

	size_t needed = size + sizeof(allocation_header_t);
	while (heap_end - heap_used < needed) {
		heap_end = (uint8_t*) sbrk(0x10000);
	}

	allocation_header_t* header = (allocation_header_t*) heap_used;
	header->size = size;
	header->times_freed = 0;
	header->magic = MAGIC;
	heap_used += needed;

	heap_lock = 0;
	return ((uint8_t*) header) + sizeof(allocation_header_t);
}

/**
 *
 */
void* realloc(void* mem, size_t size) {

	if (mem == 0) {
		mem = malloc(size);
	}

	allocation_header_t* header = (allocation_header_t*) ((uint8_t*) mem
			- sizeof(allocation_header_t));
	void* copy = malloc(size);
	memcpy(copy, mem, header->size);
	free(mem);
	return copy;
}

/**
 *
 */
void* calloc(size_t num, size_t size) {

	void* mem = malloc(num * size);
	if (mem == NULL) {
		return NULL;
	}

	memset(mem, 0, num * size);
	return mem;
}
