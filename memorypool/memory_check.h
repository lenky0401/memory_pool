#ifndef _MEMORY_CHECK_H_INCLUDED_
#define _MEMORY_CHECK_H_INCLUDED_

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/**
 * @brief 检查运行过程中，元信息状态是否正确
 * @param pool 内存池指针
 * @return void
 */
void check_meta_running_state(memory_pool *pool);

/**
 * @brief 检查内存完全释放后，元信息完整状态是否正确
 * @param pool 内存池指针
 * @return void
 */
void check_meta_complete_state(memory_pool *pool);


#endif
