﻿#ifndef _MEMORY_COMMON_H_INCLUDED_
#define _MEMORY_COMMON_H_INCLUDED_

#include "memory_pool.h"

#define TRUE 1
#define FALSE 0

#define MEM_ADDR_ALIGN_PADDING (sizeof(void*) + (MP_CACHELINE_SIZE - 1))

static void* os_malloc(uint32_t size)
{
    return malloc(size);
}

static void os_free(void *ptr)
{
    free(ptr);
}

/**
 * 对addr_ori地址做对齐，调用该函数的地方需确保内存空间足够（需多申请
 * MEM_ADDR_ALIGN_PADDING字节），否则可能导致越界。
 */
static void* mem_addr_align(void *addr_ori)
{
    void *addr_align = (void*)(((uintptr_t)addr_ori + 
        MEM_ADDR_ALIGN_PADDING) & (~(MP_CACHELINE_SIZE - 1)));

    void **ref = (void**)((uintptr_t)addr_align - sizeof(void*));
    *ref = addr_ori;

    return addr_align;
}

//返回实际分配的地址
static void* mem_addr_ori(void *addr_align)
{
    return ((void**)(addr_align))[-1];
}

static void* os_malloc_align(uint32_t size)
{
    char *addr_ori = (char *)os_malloc(MEM_ADDR_ALIGN_PADDING + size);
    if (!addr_ori) {
        return NULL;
    }

    return mem_addr_align(addr_ori);
}

static void os_free_align(void *ptr)
{
    os_free(mem_addr_ori(ptr));
}

static uint32_t get_align_order(uint32_t size)
{
    if (size == 0) {
        return 0;
    }

    uint32_t num = (((size) + MP_CACHELINE_SIZE - 1) / MP_CACHELINE_SIZE);
    assert(num >= 1);

    int32_t order = (num & (num - 1)) ? 0 : -1;
    while (num) {
        num >>= 1;
        order++;
    }

    return order;
}

static uint32_t get_seg_item_size(seg_item *item)
{
    assert(item != NULL);
    uint32_t size = item->linear_addr_offset_end - item->linear_addr_offset_start;
    assert(size > sizeof(seg_item));
    return size;
}

#endif