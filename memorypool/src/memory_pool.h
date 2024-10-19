#ifndef _MEMORY_POOL_H_INCLUDED_
#define _MEMORY_POOL_H_INCLUDED_

#ifdef __cplusplus
//extern "C" {
#endif //_cplusplus

#include "memory_pool_inner.h"

/**
 * @brief 创建内存池
 * @param size 内存池大小，单位为字节
 * @param thread_safe 是否线程安全
 * @return 内存池指针
 */
memory_pool* memory_pool_create(uint32_t size, int32_t need_thread_safe);

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
int memory_pool_free(memory_pool *pool, void *ptr);

/**
 * @brief 内存分片释放
 * @param pool 内存池指针
 * @param ptr 待分片释放的内存头指针
 * @param info 待分片释放的内存指针
 * @return 见：SLICE_FREE_RET_*
 *         释放成功后，切分的分片内存块存在参数info里返回出来。
 */
//一次最多释放8块分片内存
#define SLICE_INFO_MAX_NUM (8)
typedef struct slice_info {
	void* ptr;		//分片内存的起始地址
	int32_t size;	//分片内存的大小，必须大于等于SLICE_FREE_MIN_SIZE
} slice_info;
typedef struct slice_info_array {
	int32_t num;	//需要释放几块分片内存
	slice_info slice_arr[SLICE_INFO_MAX_NUM + 1];	//每块分片内存的起始地址指针
	                                    //释放中间内存后，返回的内存被切开，因此数量+1
} slice_info_array;
#define SLICE_FREE_RET_Ok 0
#define SLICE_FREE_RET_Not_Supported 1
#define SLICE_FREE_RET_Bad_parameter 2
#define SLICE_FREE_RET_Bad_sliceinfo 3
int memory_pool_slice_free(memory_pool *pool, void *ptr, slice_info_array *info);

#ifdef __cplusplus
//}
#endif //_cplusplus

#endif
