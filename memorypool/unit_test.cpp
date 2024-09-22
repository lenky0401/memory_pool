
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include "unit_test.h"
#include "memory_pool_inner.h"
#include "memory_pool.h"

#include "memory_common.h"

void test_os_malloc_free()
{
	void *p1 = os_malloc_align(1);
	assert((size_t)p1 % MP_CACHELINE_SIZE == 0);
	os_free_align(p1);

	p1 = os_malloc_align(10);
	assert((size_t)p1 % MP_CACHELINE_SIZE == 0);
	os_free_align(p1);

	p1 = os_malloc_align(100);
	assert((size_t)p1 % MP_CACHELINE_SIZE == 0);
	os_free_align(p1);

	p1 = os_malloc_align(1000);
	assert((size_t)p1 % MP_CACHELINE_SIZE == 0);
	os_free_align(p1);

	p1 = os_malloc_align(10000);
	assert((size_t)p1 % MP_CACHELINE_SIZE == 0);
	os_free_align(p1);
}

void test_get_align_order()
{
	assert(get_align_order(MP_CACHELINE_SIZE / 2) == 0);
	assert(get_align_order(MP_CACHELINE_SIZE) == 0);
	assert(get_align_order(MP_CACHELINE_SIZE * 2) == 1);
	assert(get_align_order(MP_CACHELINE_SIZE * 3) == 2);
	assert(get_align_order(MP_CACHELINE_SIZE * 4) == 2);
	assert(get_align_order(MP_CACHELINE_SIZE * 512) == 9);
	assert(get_align_order(MP_CACHELINE_SIZE * 1024) == 10);
}

void test_malloc()
{
	memory_pool *pool = memory_pool_create(10000, false);
	assert(pool);
	check_meta_complete_state(pool);

	void *p = memory_pool_malloc(pool, 100);
	assert(p);
	check_meta_running_state(pool);

	memory_pool_free(pool, p);
	check_meta_complete_state(pool);

	void *p1 = memory_pool_malloc(pool, 100);
	assert(p1);
	void *p2 = memory_pool_malloc(pool, 8000);
	assert(p2);
	void *p3 = memory_pool_malloc(pool, 1000);
	assert(p3);
	void *p4 = memory_pool_malloc(pool, 800);
	assert(p4);
	check_meta_running_state(pool);

	void *p5 = memory_pool_malloc(pool, 1000);
	assert(p5);
	check_meta_running_state(pool);

	memory_pool_free(pool, p4);
	check_meta_running_state(pool);
	memory_pool_free(pool, p5);
	check_meta_running_state(pool);
	memory_pool_free(pool, p1);
	check_meta_running_state(pool);
	memory_pool_free(pool, p2);
	memory_pool_free(pool, p3);

	check_meta_complete_state(pool);
	memory_pool_destroy(pool);
}

void test_part_free()
{
	memory_pool *pool = memory_pool_create(10000, false);
	assert(pool);
	check_meta_complete_state(pool);

	void *p = memory_pool_malloc(pool, 8000);
	assert(p);
	check_meta_running_state(pool);

	part_info_array info;
	info.num = 2;
	info.part_arr[0].part_ptr = (uint8_t *)p + 100;
	info.part_arr[0].part_size = 500;
	info.part_arr[1].part_ptr = (uint8_t *)p + 1000;
	info.part_arr[1].part_size = 500;

	int ret = memory_pool_part_free(pool, p, &info);
	assert(ret == PART_FREE_RET_Ok);
	check_meta_running_state(pool);

	check_meta_complete_state(pool);
	memory_pool_destroy(pool);
}
