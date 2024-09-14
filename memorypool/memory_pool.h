#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/**
 * @brief 创建内存池
 * @param size 内存池大小，单位为字节
 * @param thread_safe 是否线程安全
 * @return 内存池指针
 */
memory_pool* memory_pool_create(uint32_t size, bool thread_safe);

/**
 * @brief 销毁内存池
 * @param pool 内存池指针
 * @return void
 */
void memory_pool_destroy(memory_pool *pool);

/**
 * @brief 内存申请
 * @param pool 内存池指针
 * @param size 申请内存大小，单位为字节
 * @return 申请的内存指针，申请失败返回NULL
 */
void* memory_pool_malloc(memory_pool *pool, uint32_t size);

/**
 * @brief 内存释放
 * @param pool 内存池指针
 * @param ptr 待释放的内存指针
 * @return void
 */
void memory_pool_free(memory_pool *pool, void *ptr);

/**
 * @brief 内存部分释放
 * @param pool 内存池指针
 * @param ptr 待部分释放的内存头指针
 * @param info 待部分释放的内存指针
 * @return 见：PART_FREE_RET_*
 *         释放成功后，切分的部分内存块存在参数info里返回出来。
 */
//一次最多释放8块部分内存
#define PART_INFO_MAX_NUM (8)
typedef struct part_info {
	void* part_ptr;		//部分内存的起始地址
	int32_t part_size;	//部分内存的大小，必须大于等于MP_ALIGN_SIZE
} part_info;
typedef struct part_info_array {
	int32_t num;	//需要释放几块部分内存
	part_info part_arr[PART_INFO_MAX_NUM + 1];	//每块部分内存的起始地址指针
	                                    //释放中间内存后，返回的内存被切开，因此数量+1
} part_info_array;
#define PART_FREE_RET_Ok 0
#define PART_FREE_RET_Not_Supported 1
#define PART_FREE_RET_Bad_parameter 2
#define PART_FREE_RET_Bad_partinfo 3
int memory_pool_part_free(memory_pool *pool, void *ptr, part_info_array *info);
