#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <malloc.h>

#include "memory_check.h"
#include "unit_test.h"

int main()
{
	test_os_malloc_free();

	test_get_align_order();

	test_malloc();

	test_slice_free();

	test_multi_thread();

	printf("ok\n");
	getchar();
	return 0;
}
