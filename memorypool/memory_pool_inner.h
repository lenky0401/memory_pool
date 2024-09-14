#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define mp_print printf
#define MP_MAX_SIZE (120 * 1024 * 1024)

#define os_malloc malloc
#define os_free free

#define MP_ALIGN_SIZE (1024)
#define MP_MEMORY_SIZE_ALIGN(size) (((size) + MP_ALIGN_SIZE - 1) / MP_ALIGN_SIZE * MP_ALIGN_SIZE)

#define SEG_ITEM_MAGIC 0x12345678

#define STAT_IN_USE (1 << 0)
#define STAT_IN_FREE (1 << 1)

#define SEG_ITEM_INVALID_VALUE -1


typedef struct seg_item {
	//����Žṹ����ǰ�棬��seg_head����һ��
	seg_item *free_list_prev;	//�����ڴ�����
	seg_item *free_list_next;	//�����ڴ�����

	uint32_t magic;
	uint32_t state;

	uint32_t linear_addr_offset_start;	//��ǰseg�����ڴ�εĿ�ʼ���Ե�ַ������seg�ṹ���С����
	uint32_t linear_addr_offset_end;	//��ǰseg�����ڴ�εĽ������Ե�ַ������seg�ṹ���С����

	seg_item *linear_addr_list_prev;	//���Ե�ַ����
	seg_item *linear_addr_list_next;	//���Ե�ַ����
} seg_item;

typedef struct seg_head {
	seg_item *free_list_prev;	//�����ڴ�����
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
