#ifndef _MEMORY_POOL_INNER_H_INCLUDED_
#define _MEMORY_POOL_INNER_H_INCLUDED_

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define mp_print printf

/**
 * 最大支持1G内存的管理
 */
#define MP_MAX_SIZE (1024 * 1024 * 1024)

//本内存池均按16字节对齐
#define MP_CACHELINE_SIZE (16)

#define SEG_ITEM_MAGIC 0x12345678

#define STAT_IN_USE (1 << 0)
#define STAT_IN_FREE (1 << 1)

#define SEG_ITEM_INVALID_VALUE (-1)

typedef struct seg_item {
	//必须放结构体最前面，和seg_head保持一致
	seg_item *free_list_prev;	//空闲内存链表
	seg_item *free_list_next;	//空闲内存链表

	uint32_t magic;
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
	bool thread_safe;
	uint32_t memory_max_size;
	uint32_t memory_max_order;
	uint8_t *memory_addr;

	seg_head *seg_head_arr;
} memory_pool;


/**
 * 部分释放的最小字节，如果小于这个字节，释放之后都不够存放一个seg_item，
 * 导致无法管理，因此做部分释放时，必须大于这个字节数。
 */
#define PART_FREE_MIN_SIZE (sizeof(seg_item))

#endif