#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>


#define mp_print printf
#define MP_MAX_SIZE (120 * 1024 * 1024)

#define os_malloc malloc
#define os_free free

#define MP_ALIGN_SIZE (64)
#define MP_MEMORY_SIZE_ALIGN(size) (((size) + MP_ALIGN_SIZE - 1) / MP_ALIGN_SIZE * MP_ALIGN_SIZE)

#define _DEBUG_
#define SEG_ITEM_MAGIC 0x12345678

#define STAT_IN_USE (1 << 0)
#define STAT_IN_FREE (1 << 1)

typedef struct seg_item {
	//必须放结构体最前面，和seg_head保持一致
	seg_item *free_list_prev;	//空闲内存链表
	seg_item *free_list_next;	//空闲内存链表

#ifdef _DEBUG_
	uint32_t magic;
#endif

	int state;

	int linear_addr_offset_start;		//当前seg的开始位置，包含seg结构体大小在内
	int linear_addr_offset_end;		//当前seg的结束位置，包含seg结构体大小在内

	seg_item *linear_addr_list_prev;
	seg_item *linear_addr_list_next;
} seg_item;

typedef struct seg_head {
	seg_item *free_list_prev;	//空闲内存链表
	seg_item *free_list_next;
} seg_head;

typedef struct memory_pool{
	uint32_t memory_max_size;
	uint32_t memory_max_order;
	uint8_t *memory_addr;

	seg_head *seg_head_arr;
} memory_pool;

uint32_t get_align_order(uint32_t size);
void check_memory_pool_meta(memory_pool *pool);

memory_pool* memory_pool_create(uint32_t size, bool thread_safe);
void memory_pool_destroy(memory_pool *pool);

void* memory_pool_malloc(memory_pool *pool, uint32_t size);
void memory_pool_free(memory_pool *pool, void *p);

