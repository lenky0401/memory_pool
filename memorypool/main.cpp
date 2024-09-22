#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <malloc.h>

#include "memory_pool_inner.h"
#include "memory_pool.h"

#include "memory_common.h"

#include "unit_test.h"


int main()
{
	test_os_malloc_free();

	test_get_align_order();

	test_malloc();

	test_part_free();

	printf("ok\n");
	getchar();
	return 0;
}
