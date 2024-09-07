
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include "memory_pool.h"

uint32_t get_align_order(uint32_t size)
{
	assert(size > 0);

	uint32_t num = (size + MP_ALIGN_SIZE - 1) / MP_ALIGN_SIZE;
	assert(num > 0);
	if (num == 1) {
		return 0;
	}

	uint32_t order = 0;
	//算2的幂，固定值
	num = (num + 1) / 2 * 2;
	while (num > 1) {
		num = num >> 1;
		order++;
	}

	return order;
}

static uint32_t get_seg_item_size(seg_item *item)
{
	assert(item != NULL);
	uint32_t size = item->linear_addr_offset_end - item->linear_addr_offset_start;
	assert(size >= 0);
	return size;
}

static void check_linear_meta_running_state(memory_pool *pool)
{
	seg_item *head = (seg_item *)pool->memory_addr;

	uint32_t size = 0;
	seg_item *item = head;
	do {
		assert(item->magic == SEG_ITEM_MAGIC);
		assert(item->state == STAT_IN_USE || item->state == STAT_IN_FREE);

		assert(item->linear_addr_list_prev->linear_addr_offset_end == item->linear_addr_offset_start ||
			(item->linear_addr_list_prev->linear_addr_offset_end == pool->memory_max_size &&
				(item->linear_addr_offset_start == 0)));

		assert(item->linear_addr_list_next->linear_addr_offset_start == item->linear_addr_offset_end ||
			(item->linear_addr_list_next->linear_addr_offset_start == 0 &&
				(item->linear_addr_offset_end == pool->memory_max_size)));

		size += get_seg_item_size(item);

		item = item->linear_addr_list_next;
	} while (item != head);
	assert(size == pool->memory_max_size);

	size = 0;
	item = head;
	do {
		assert(item->magic == SEG_ITEM_MAGIC);
		assert(item->state == STAT_IN_USE || item->state == STAT_IN_FREE);

		assert(item->linear_addr_list_prev->linear_addr_offset_end == item->linear_addr_offset_start ||
			(item->linear_addr_list_prev->linear_addr_offset_end == pool->memory_max_size &&
				(item->linear_addr_offset_start == 0)));

		assert(item->linear_addr_list_next->linear_addr_offset_start == item->linear_addr_offset_end ||
			(item->linear_addr_list_next->linear_addr_offset_start == 0 &&
				(item->linear_addr_offset_end == pool->memory_max_size)));

		size += get_seg_item_size(item);

		item = item->linear_addr_list_prev;
	} while (item != head);
	assert(size == pool->memory_max_size);
}

void check_free_meta_running_state(memory_pool *pool)
{
	seg_item *head, *item;
	uint32_t order;

	for (uint32_t i = 0; i <= pool->memory_max_order; i++) {

		head = (seg_item *)&(pool->seg_head_arr[i]);
		item = head->free_list_next;
		while (item != head) {
			assert(item->magic == SEG_ITEM_MAGIC);
			assert(item->state == STAT_IN_FREE);

			order = get_align_order(get_seg_item_size(item));
			assert(order == i);

			item = item->free_list_next;
		}

		item = head->free_list_prev;
		while (item != head) {
			assert(item->magic == SEG_ITEM_MAGIC);
			assert(item->state == STAT_IN_FREE);

			order = get_align_order(get_seg_item_size(item));
			assert(order == i);

			item = item->free_list_prev;
		}
	}
}

void check_meta_running_state(memory_pool *pool)
{
	check_linear_meta_running_state(pool);
	check_free_meta_running_state(pool);
}

//检查元信息是完整状态
void check_meta_complete_state(memory_pool *pool)
{
	seg_item *head = (seg_item *)pool->memory_addr;
	assert(get_seg_item_size(head) == pool->memory_max_size);
	assert(head->magic == SEG_ITEM_MAGIC);
	assert(head->state == STAT_IN_FREE);
	assert(head->linear_addr_list_next == head && head->linear_addr_list_prev == head);

	uint32_t order = get_align_order(get_seg_item_size(head));
	seg_item *free_head = (seg_item *)&(pool->seg_head_arr[order]);
	assert(free_head->free_list_next == head);
	assert(free_head->free_list_prev == head);
	assert(head->free_list_next == free_head);
	assert(head->free_list_prev == free_head);

	for (uint32_t i = 0; i <= pool->memory_max_order; i++) {
		if (i == order) {
			continue;
		}

		free_head = (seg_item *)&(pool->seg_head_arr[i]);
		assert(free_head->free_list_next == free_head);
		assert(free_head->free_list_prev == free_head);
	}
}

void seg_free_order_add(memory_pool *pool, seg_item *item)
{
	uint32_t order = get_align_order(get_seg_item_size(item));

	assert(order >= 0 && order <= pool->memory_max_order);
	seg_head *head = &(pool->seg_head_arr[order]);

	item->free_list_next = head->free_list_next;
	head->free_list_next = item;
	item->free_list_next->free_list_prev = item;
	item->free_list_prev = (seg_item *)head;
}

bool is_memory_pool_addr(memory_pool *pool, void *ptr)
{
	if ((uint64_t)pool->memory_addr <= (uint64_t)ptr &&
		(uint64_t)ptr < (uint64_t)pool->memory_addr + pool->memory_max_size) {
		return true;
	}
	return false;
}

memory_pool* memory_pool_create(uint32_t size, bool thread_safe)
{
	memory_pool *pool = NULL;

	if (size == 0 || size > MP_MAX_SIZE) {
		mp_print("para size is error: %d", size);
		return NULL;
	}
	size = MP_MEMORY_SIZE_ALIGN(size);

	pool = (memory_pool *)os_malloc(sizeof(memory_pool));
	if (!pool) {
		return NULL;
	}
	memset(pool, 0, sizeof(memory_pool));

	pool->memory_max_size = size;
	pool->memory_addr = (uint8_t *)os_malloc(size);
	if (!pool->memory_addr) {
		goto err;
	}

	pool->memory_max_order = get_align_order(size);
	pool->seg_head_arr = (seg_head *)os_malloc((pool->memory_max_order + 1)
		* sizeof(seg_head));
	if (!pool->seg_head_arr) {
		goto err;
	}
	for (uint32_t i = 0; i <= pool->memory_max_order; i++) {
		pool->seg_head_arr[i].free_list_prev = 
			pool->seg_head_arr[i].free_list_next = (seg_item *)&(pool->seg_head_arr[i]);
	}

	seg_item *item = (seg_item *)pool->memory_addr;
	item->linear_addr_offset_start = 0;
	item->linear_addr_offset_end = size;
	item->linear_addr_list_next = item->linear_addr_list_prev = item;
	item->state = STAT_IN_FREE;
#ifdef _DEBUG_
	item->magic = SEG_ITEM_MAGIC;
#endif

	seg_free_order_add(pool, item);

	return pool;

err:
	memory_pool_destroy(pool);
	return NULL;
}

void memory_pool_destroy(memory_pool *pool)
{
	if (!pool) {
		return;
	}

	if (pool->seg_head_arr) {
		os_free(pool->seg_head_arr);
	}

	if (pool->memory_addr) {
		os_free(pool->memory_addr);
	}

	os_free(pool);
}

void* memory_pool_malloc(memory_pool *pool, uint32_t size)
{
	if (size == 0) {
		return NULL;
	}
	if (pool == NULL || size > MP_MAX_SIZE) {
		return os_malloc(size);
	}

	uint32_t need_size = MP_MEMORY_SIZE_ALIGN(size + sizeof(seg_item));
	uint32_t need_order = get_align_order(need_size);
	for (uint32_t curt_order = need_order; curt_order <= pool->memory_max_order; curt_order++) {

		seg_head *head = &(pool->seg_head_arr[curt_order]);
		seg_item *item = (seg_item *)head->free_list_next;
		if (item == (seg_item *)head) {
			continue;
		}

		while (item != (seg_item *)head) {

			assert(item->state == STAT_IN_FREE);

			if (get_seg_item_size(item) >= need_size) {
				break;
			}
			item = (seg_item *)item->free_list_next;
		}

		//还是没找到可用空间
		if (item == (seg_item *)head) {
			continue;
		}

		//先从free链表里拆出来
		item->free_list_next->free_list_prev = item->free_list_prev;
		item->free_list_prev->free_list_next = item->free_list_next;

		//如果大小刚和足够，直接返回
		if (get_seg_item_size(item) == need_size) {
			item->state = STAT_IN_USE;
			return item + 1;
		}

		//如果有多的空间，需分出来
		assert(get_seg_item_size(item) > need_size);
		seg_item *ret_item = item;
		item = (seg_item *)((uint8_t *)ret_item + need_size);
		item->linear_addr_offset_start = ret_item->linear_addr_offset_start + need_size;
		item->linear_addr_offset_end = ret_item->linear_addr_offset_end;
		ret_item->linear_addr_offset_end = item->linear_addr_offset_start;

		item->linear_addr_list_next = ret_item->linear_addr_list_next;
		ret_item->linear_addr_list_next = item;
		item->linear_addr_list_prev = ret_item;
		item->linear_addr_list_next->linear_addr_list_prev = item;

		item->state = STAT_IN_FREE;
#ifdef _DEBUG_
		item->magic = SEG_ITEM_MAGIC;
#endif
		//分出来的空间保存到free链表
		seg_free_order_add(pool, item);

		//返回
		ret_item->state = STAT_IN_USE;
		return ret_item + 1;
	}

	return os_malloc(size);
}

void memory_pool_free(memory_pool *pool, void *ptr)
{
	if (!is_memory_pool_addr(pool, ptr)) {
		os_free(ptr);
		return;
	}

	seg_item *curt = (seg_item *)((uint8_t *)ptr - sizeof(seg_item));
#ifdef _DEBUG_
	assert(curt->magic == SEG_ITEM_MAGIC);
#endif

	seg_item *prev = curt->linear_addr_list_prev;
	seg_item *next = curt->linear_addr_list_next;

	assert(prev->magic == SEG_ITEM_MAGIC);
	assert(prev->state == STAT_IN_USE || prev->state == STAT_IN_FREE);
	assert(prev->linear_addr_offset_end == curt->linear_addr_offset_start ||
		(prev->linear_addr_offset_end == pool->memory_max_size && 
			(curt->linear_addr_offset_start == 0)));

	assert(next->magic == SEG_ITEM_MAGIC);
	assert(next->state == STAT_IN_USE || next->state == STAT_IN_FREE);
	assert(next->linear_addr_offset_start == curt->linear_addr_offset_end);

	curt->state = STAT_IN_FREE;

	bool prev_merge = prev != curt && prev->state == curt->state &&
		prev->linear_addr_offset_end == curt->linear_addr_offset_start;
	bool next_merge = next != curt && next->state == curt->state &&
		next->linear_addr_offset_start == curt->linear_addr_offset_end;

	//与前后块都能合并
	if (prev_merge && next_merge)
	{
		//把上一块的空间拉大
		prev->linear_addr_offset_end = next->linear_addr_offset_end;

		//把当前块和下一块从linear链表删除
		prev->linear_addr_list_next = next->linear_addr_list_next;
		next->linear_addr_list_next->linear_addr_list_prev = prev;

		//把上一块和下一块都从free链表删除
		prev->free_list_prev->free_list_next = prev->free_list_next;
		prev->free_list_next->free_list_prev = prev->free_list_prev;
		next->free_list_prev->free_list_next = next->free_list_next;
		next->free_list_next->free_list_prev = next->free_list_prev;

		//把上一块加入free链表
		seg_free_order_add(pool, prev);
	}

	//与前块能合并
	else if (prev_merge)
	{
		//把上一块的空间拉大
		prev->linear_addr_offset_end = curt->linear_addr_offset_end;

		//把当前块从linear链表删除
		prev->linear_addr_list_next = curt->linear_addr_list_next;
		curt->linear_addr_list_next->linear_addr_list_prev = prev;

		//把上一块从free链表删除
		prev->free_list_prev->free_list_next = prev->free_list_next;
		prev->free_list_next->free_list_prev = prev->free_list_prev;

		//把上一块加入free链表
		seg_free_order_add(pool, prev);
	}

	//与后块能合并
	else if (next_merge)
	{
		//把当前块的空间拉大
		curt->linear_addr_offset_end = next->linear_addr_offset_end;

		//把下一块从linear链表删除
		curt->linear_addr_list_next = next->linear_addr_list_next;
		next->linear_addr_list_next->linear_addr_list_prev = curt;

		//把下一块从free链表删除
		next->free_list_prev->free_list_next = next->free_list_next;
		next->free_list_next->free_list_prev = next->free_list_prev;

		//把当前块加入free链表
		seg_free_order_add(pool, curt);
	}

	//与前后块都不能合并
	else {
		//把当前块加入linear链表
		prev->linear_addr_list_next = curt;
		curt->linear_addr_list_prev = prev;
		curt->linear_addr_list_next = next;
		next->linear_addr_list_prev = curt;

		//把当前块加入free链表
		seg_free_order_add(pool, curt);
	}
}

bool memory_pool_part_free(memory_pool *pool, void *ptr, part_info *info)
{
	if (!is_memory_pool_addr(pool, ptr)) {
		os_free(ptr);
		return;
	}


}