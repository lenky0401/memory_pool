#ifndef _MEMORY_POOL_INNER_H_INCLUDED_
#define _MEMORY_POOL_INNER_H_INCLUDED_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#ifdef _POSIX_THREADS
#include <pthread.h>
#else
#include <mutex>
#endif

#define mp_print printf

/**
 * 最大支持1G内存的管理
 */
#define MP_MAX_SIZE (1024 * 1024 * 1024)

//本内存池均按16字节对齐
#define MP_CACHELINE_SIZE (16)

#define SEG_ITEM_MAGIC 0x12345678

//空闲中
#define STAT_IN_FREE (1 << 0)
//使用中
#define STAT_IN_USE (1 << 1)

#define SEG_ITEM_INVALID_VALUE (-1)

typedef struct seg_item {
	//必须放结构体最前面，和seg_head保持一致
	struct seg_item *free_list_prev;	//空闲内存链表
	struct seg_item *free_list_next;	//空闲内存链表

	uint32_t magic;
	uint32_t state;

	uint32_t linear_addr_offset_start;	//当前seg管理内存段的开始线性地址，包含seg结构体大小在内
	uint32_t linear_addr_offset_end;	//当前seg管理内存段的结束线性地址，包含seg结构体大小在内

	struct seg_item *linear_addr_list_prev;	//线性地址链表
	struct seg_item *linear_addr_list_next;	//线性地址链表
} seg_item;

typedef struct seg_head {
	seg_item *free_list_prev;	//空闲内存链表
	seg_item *free_list_next;
} seg_head;

typedef struct memory_pool{
	int32_t need_thread_safe;
#ifdef _POSIX_THREADS
	pthread_mutex_t mutex;
#else
	std::mutex mutex;
#endif
	uint32_t memory_max_size;
	uint32_t memory_max_order;
	uint8_t *memory_addr;

	seg_head *seg_head_arr;
} memory_pool;

#endif