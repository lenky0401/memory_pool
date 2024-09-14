#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <malloc.h>

#include "memory_pool_inner.h"
#include "memory_pool.h"

void test()
{
	assert(get_align_order(64) == 0);
	assert(get_align_order(32) == 0);
	assert(get_align_order(64) == 0);
}

int main()
{
	test();

	memory_pool *pool = memory_pool_create(10000, 1);

	check_meta_running_state(pool);
	check_meta_complete_state(pool);

	void *p = memory_pool_malloc(pool, 100);
	check_meta_running_state(pool);

	memory_pool_free(pool, p);
	check_meta_running_state(pool);
	check_meta_complete_state(pool);

	void *p1 = memory_pool_malloc(pool, 100);
	void *p2 = memory_pool_malloc(pool, 8000);
	void *p3 = memory_pool_malloc(pool, 1000);
	void *p4 = memory_pool_malloc(pool, 800);

	check_meta_running_state(pool);

	memory_pool_free(pool, p3);

	void *p5 = memory_pool_malloc(pool, 1000);

	check_meta_running_state(pool);

	memory_pool_free(pool, p4);

	check_meta_running_state(pool);

	memory_pool_free(pool, p5);

	check_meta_running_state(pool);

	memory_pool_free(pool, p1);

	check_meta_running_state(pool);

	memory_pool_free(pool, p2);

	printf("ok\n");
	getchar();
	return 0;
}
