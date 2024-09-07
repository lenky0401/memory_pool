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

	uint32_t state;

	uint32_t linear_addr_offset_start;	//当前seg管理内存段的开始线性地址，包含seg结构体大小在内
	uint32_t linear_addr_offset_end;	//当前seg管理内存段的结束线性地址，包含seg结构体大小在内

	seg_item *linear_addr_list_prev;	//线性地址链表
	seg_item *linear_addr_list_next;	//线性地址链表
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
void check_meta_running_state(memory_pool *pool);
void check_meta_complete_state(memory_pool *pool);

memory_pool* memory_pool_create(uint32_t size, bool thread_safe);
void memory_pool_destroy(memory_pool *pool);

void* memory_pool_malloc(memory_pool *pool, uint32_t size);
void memory_pool_free(memory_pool *pool, void *ptr);

#define PART_INFO_MAX_NUM (8)
typedef struct part_info {
	uint32_t num;
	void* ptr[PART_INFO_MAX_NUM + 1];
} part_info;

bool memory_pool_part_free(memory_pool *pool, void *ptr, part_info *info);
