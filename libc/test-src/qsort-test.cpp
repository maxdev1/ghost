#include <iostream>
#include <time.h>

#include "../src/stdlib/qsort.c"

int test_comp(const void* a, const void* b)
{
	uint32_t va = *((uint32_t*) a);
	uint32_t vb = *((uint32_t*) b);

	if(va == vb)
		return 0;
	if(va < vb)
		return -1;
	return 1;
}

/**
 *
 */
void test_qsort()
{
	size_t items = 32;
	uint32_t* test = new uint32_t[items];
	for(int i = 0; i < items; i++)
	{
		test[i] = rand() * 0.00002;
		printf("%i, ", test[i]);
	}
	printf("\n");

	qsort(test, items, sizeof(uint32_t), test_comp);
	for(int i = 0; i < items; i++)
	{
		printf("%i, ", test[i]);
	}
	printf("\n");
}

struct sortable_t
{
	int v;
	int a;
	int b;
	uint8_t big[4096];
	int c;
};

int test_comp_struct(const void* a, const void* b)
{
	sortable_t* va = ((sortable_t*) a);
	sortable_t* vb = ((sortable_t*) b);

	if(va->v == vb->v)
		return 0;
	if(va->v < vb->v)
		return -1;
	return 1;
}
/**
 *
 */
void test_qsort_struct()
{
	srand(time(NULL));
	size_t items = 32;
	sortable_t* test = new sortable_t[items];
	for(int i = 0; i < items; i++)
	{
		test[i].v = rand() * 0.0002;
		test[i].a = 1;
		test[i].b = 2;
		test[i].c = 3;
		for(int j = 0; j < 4096; j++)
		{
			test[i].big[j] = 0xFA;
		}
		printf("%i, ", test[i].v);
	}
	printf("\n");

	qsort(test, items, sizeof(sortable_t), test_comp);
	int l = 0;
	for(int i = 0; i < items; i++)
	{
		if(l > test[i].v)
		{
			fprintf(stderr, "sort failed @%i: %i > %i\n", i, l, test[i].v);
			return;
		}
		if(test[i].a != 1 || test[i].b != 2 || test[i].c != 3)
		{
			fprintf(stderr, "swap failed\n");
			return;
		}
		for(int j = 0; j < 4096; j++)
		{
			if(test[i].big[j] != 0xFA)
			{
				fprintf(stderr, "swap failed @%i @%i -> %x\n", i, j, test[i].big[j]);
				return;
			}
		}
		l = test[i].v;
		printf("%i, ", test[i].v);
	}
	printf("\n");
}

/**
 *
 */
int main(int argc, char** argv)
{
	test_qsort();
	test_qsort_struct();
}
