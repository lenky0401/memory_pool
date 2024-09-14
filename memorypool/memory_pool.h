#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/**
 * @brief �����ڴ��
 * @param size �ڴ�ش�С����λΪ�ֽ�
 * @param thread_safe �Ƿ��̰߳�ȫ
 * @return �ڴ��ָ��
 */
memory_pool* memory_pool_create(uint32_t size, bool thread_safe);

/**
 * @brief �����ڴ��
 * @param pool �ڴ��ָ��
 * @return void
 */
void memory_pool_destroy(memory_pool *pool);

/**
 * @brief �ڴ�����
 * @param pool �ڴ��ָ��
 * @param size �����ڴ��С����λΪ�ֽ�
 * @return ������ڴ�ָ�룬����ʧ�ܷ���NULL
 */
void* memory_pool_malloc(memory_pool *pool, uint32_t size);

/**
 * @brief �ڴ��ͷ�
 * @param pool �ڴ��ָ��
 * @param ptr ���ͷŵ��ڴ�ָ��
 * @return void
 */
void memory_pool_free(memory_pool *pool, void *ptr);

/**
 * @brief �ڴ沿���ͷ�
 * @param pool �ڴ��ָ��
 * @param ptr �������ͷŵ��ڴ�ͷָ��
 * @param info �������ͷŵ��ڴ�ָ��
 * @return ����PART_FREE_RET_*
 *         �ͷųɹ����зֵĲ����ڴ����ڲ���info�ﷵ�س�����
 */
//һ������ͷ�8�鲿���ڴ�
#define PART_INFO_MAX_NUM (8)
typedef struct part_info {
	void* part_ptr;		//�����ڴ����ʼ��ַ
	int32_t part_size;	//�����ڴ�Ĵ�С��������ڵ���MP_ALIGN_SIZE
} part_info;
typedef struct part_info_array {
	int32_t num;	//��Ҫ�ͷż��鲿���ڴ�
	part_info part_arr[PART_INFO_MAX_NUM + 1];	//ÿ�鲿���ڴ����ʼ��ַָ��
	                                    //�ͷ��м��ڴ�󣬷��ص��ڴ汻�п����������+1
} part_info_array;
#define PART_FREE_RET_Ok 0
#define PART_FREE_RET_Not_Supported 1
#define PART_FREE_RET_Bad_parameter 2
#define PART_FREE_RET_Bad_partinfo 3
int memory_pool_part_free(memory_pool *pool, void *ptr, part_info_array *info);
