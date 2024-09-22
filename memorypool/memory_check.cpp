
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include "memory_pool_inner.h"
#include "memory_pool.h"

#include "memory_common.h"

/**
 * 检查线性地址链表是否正确
 */
static void check_linear_meta_running_state(memory_pool *pool)
{
	seg_item *head = (seg_item *)pool->memory_addr;

	uint32_t size = 0;
	seg_item *item = head;
	do {
		assert(item->magic == SEG_ITEM_MAGIC);
		assert(item->state == STATE_FREE || item->state == STATE_USE_NOTALIGN
			|| item->state == STATE_USE_ALIGN);

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
		assert(item->state == STATE_FREE || item->state == STATE_USE_NOTALIGN
			|| item->state == STATE_USE_ALIGN);

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

/**
 * 检查空闲内存地址段链表是否正确
 */
static void check_free_meta_running_state(memory_pool *pool)
{
	seg_item *head, *item;
	uint32_t order;

	for (uint32_t i = 0; i <= pool->memory_max_order; i++) {

		head = (seg_item *)&(pool->seg_head_arr[i]);
		item = head->free_list_next;
		while (item != head) {
			assert(item->magic == SEG_ITEM_MAGIC);
			assert(item->state == STATE_FREE);

			order = get_align_order(get_seg_item_size(item));
			assert(order == i);

			item = item->free_list_next;
		}

		item = head->free_list_prev;
		while (item != head) {
			assert(item->magic == SEG_ITEM_MAGIC);
			assert(item->state == STATE_FREE);

			order = get_align_order(get_seg_item_size(item));
			assert(order == i);

			item = item->free_list_prev;
		}
	}
}

/**
 * 检查两个链表是否正确
 */
void check_meta_running_state(memory_pool *pool)
{
	check_linear_meta_running_state(pool);
	check_free_meta_running_state(pool);
}

/**
 * 在没有内存使用的情况下，元信息需全为初始状态
 */
void check_meta_complete_state(memory_pool *pool)
{
	seg_item *head = (seg_item *)pool->memory_addr;
	assert(get_seg_item_size(head) == pool->memory_max_size);
	assert(head->magic == SEG_ITEM_MAGIC);
	assert(head->state == STATE_FREE);
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
