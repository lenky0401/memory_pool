
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