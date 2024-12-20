﻿
#ifdef _WIN32
#include <Windows.h>
#endif

#include <thread>
#include <assert.h>

#include "unit_test.h"
#include "memory_check.h"
#include "../memory_common.h"

void set_mem_value(void *mem, char value, uint32_t size)
{
    memset(mem, value, size);
}

void set_slice_mem_value(slice_info_array *info, char value)
{
    for (int i = 0; i < info->num; i++) {
        slice_info *data = &(info->slice_arr[i]);
        set_mem_value(data->ptr, -1, data->size);
    }
}

void test_os_malloc_free()
{
    void *p1 = os_malloc_align(1);
    assert((size_t)p1 % MP_CACHELINE_SIZE == 0);
    os_free_align(p1);

    p1 = os_malloc_align(10);
    assert((size_t)p1 % MP_CACHELINE_SIZE == 0);
    os_free_align(p1);

    p1 = os_malloc_align(100);
    assert((size_t)p1 % MP_CACHELINE_SIZE == 0);
    os_free_align(p1);

    p1 = os_malloc_align(1000);
    assert((size_t)p1 % MP_CACHELINE_SIZE == 0);
    os_free_align(p1);

    p1 = os_malloc_align(10000);
    assert((size_t)p1 % MP_CACHELINE_SIZE == 0);
    os_free_align(p1);
}

void test_get_align_order()
{
    //0: (0, 1]
    assert(get_align_order(0) == 0);
    assert(get_align_order(1) == 0);
    assert(get_align_order((uint32_t)(MP_CACHELINE_SIZE * 0.5f)) == 0);
    assert(get_align_order((uint32_t)(MP_CACHELINE_SIZE * 0.8f)) == 0);
    assert(get_align_order((uint32_t)(MP_CACHELINE_SIZE * 1.0f)) == 0);

    //1: (1, 2]
    assert(get_align_order((uint32_t)(MP_CACHELINE_SIZE * 1.1f)) == 1);
    assert(get_align_order((uint32_t)(MP_CACHELINE_SIZE * 1.8f)) == 1);
    assert(get_align_order((uint32_t)(MP_CACHELINE_SIZE * 2.0f)) == 1);

    //2: (2, 4]
    assert(get_align_order((uint32_t)(MP_CACHELINE_SIZE * 2.1f)) == 2);
    assert(get_align_order((uint32_t)(MP_CACHELINE_SIZE * 2.8f)) == 2);
    assert(get_align_order((uint32_t)(MP_CACHELINE_SIZE * 3.0f)) == 2);
    assert(get_align_order((uint32_t)(MP_CACHELINE_SIZE * 4.0f)) == 2);

    //3: (4, 8]
    assert(get_align_order((uint32_t)(MP_CACHELINE_SIZE * 4.1f)) == 3);
    assert(get_align_order((uint32_t)(MP_CACHELINE_SIZE * 5.8f)) == 3);
    assert(get_align_order((uint32_t)(MP_CACHELINE_SIZE * 6.0f)) == 3);
    assert(get_align_order((uint32_t)(MP_CACHELINE_SIZE * 7.0f)) == 3);
    assert(get_align_order((uint32_t)(MP_CACHELINE_SIZE * 8.0f)) == 3);

    //4: (8, 16]
    assert(get_align_order((uint32_t)(MP_CACHELINE_SIZE * 8.1f)) == 4);
    assert(get_align_order((uint32_t)(MP_CACHELINE_SIZE * 9.8f)) == 4);
    assert(get_align_order((uint32_t)(MP_CACHELINE_SIZE * 12.0f)) == 4);
    assert(get_align_order((uint32_t)(MP_CACHELINE_SIZE * 14.0f)) == 4);
    assert(get_align_order((uint32_t)(MP_CACHELINE_SIZE * 16.0f)) == 4);

    //5: (16, 32]
    assert(get_align_order((uint32_t)(MP_CACHELINE_SIZE * 16.1f)) == 5);
    assert(get_align_order((uint32_t)(MP_CACHELINE_SIZE * 20.8f)) == 5);
    assert(get_align_order((uint32_t)(MP_CACHELINE_SIZE * 26.0f)) == 5);
    assert(get_align_order((uint32_t)(MP_CACHELINE_SIZE * 28.0f)) == 5);
    assert(get_align_order((uint32_t)(MP_CACHELINE_SIZE * 32.0f)) == 5);

    //n: other
    assert(get_align_order((uint32_t)(MP_CACHELINE_SIZE * 511.1f)) == 9);
    assert(get_align_order((uint32_t)(MP_CACHELINE_SIZE * 512.0f)) == 9);
    assert(get_align_order((uint32_t)(MP_CACHELINE_SIZE * 513.0f)) == 10);
    assert(get_align_order((uint32_t)(MP_CACHELINE_SIZE * 514.0f)) == 10);
    assert(get_align_order((uint32_t)(MP_CACHELINE_SIZE * 1024.0f)) == 10);
    assert(get_align_order((uint32_t)(MP_CACHELINE_SIZE * 1024.1f)) == 11);
}

void test_malloc()
{
    memory_pool *pool = memory_pool_create(10000, false);
    assert(pool);
    check_meta_complete_state(pool);

    void *p = memory_pool_malloc(pool, 100);
    assert(p);
    set_mem_value(p, -1, 100);
    check_meta_running_state(pool);

    memory_pool_free(pool, p);
    check_meta_complete_state(pool);

    void *p1 = memory_pool_malloc(pool, 100);
    assert(p1);
    set_mem_value(p1, -1, 100);
    void *p2 = memory_pool_malloc(pool, 8000);
    assert(p2);
    set_mem_value(p2, -1, 8000);
    void *p3 = memory_pool_malloc(pool, 1000);
    assert(p3);
    set_mem_value(p3, -1, 1000);
    void *p4 = memory_pool_malloc(pool, 800);
    assert(p4);
    set_mem_value(p4, -1, 800);
    check_meta_running_state(pool);

    void *p5 = memory_pool_malloc(pool, 1000);
    assert(p5);
    set_mem_value(p5, -1, 1000);
    check_meta_running_state(pool);

    memory_pool_free(pool, p4);
    check_meta_running_state(pool);
    memory_pool_free(pool, p5);
    check_meta_running_state(pool);
    memory_pool_free(pool, p1);
    check_meta_running_state(pool);
    memory_pool_free(pool, p2);
    memory_pool_free(pool, p3);

    check_meta_complete_state(pool);
    memory_pool_destroy(pool);
}

void test_slice_free()
{
    //start
    memory_pool *pool = memory_pool_create(10000, false);
    assert(pool);
    check_meta_complete_state(pool);

    //test 1
    {
        void *p = memory_pool_malloc(pool, 1000);
        assert(p);
        check_meta_running_state(pool);

        slice_info_array info;
        info.num = 2;
        info.slice_arr[0].ptr = (uint8_t *)p + 100;
        info.slice_arr[0].size = 100;
        info.slice_arr[1].ptr = (uint8_t *)p + 300;
        info.slice_arr[1].size = 100;

        int ret = memory_pool_slice_free(pool, p, &info);
        assert(ret == FREE_RET_Ok);
        set_slice_mem_value(&info, -1);
        check_meta_running_state(pool);
        for (int i = 0; i < info.num; i++) {
            memory_pool_free(pool, info.slice_arr[i].ptr);
        }
        check_meta_complete_state(pool);
    }

    //test 2
    {
        void *p = memory_pool_malloc(pool, 2000);
        assert(p);
        check_meta_running_state(pool);

        slice_info_array info;
        info.num = 2;
        info.slice_arr[0].ptr = (uint8_t *)p;
        info.slice_arr[0].size = 100;
        info.slice_arr[1].ptr = (uint8_t *)p + 100;
        info.slice_arr[1].size = 100;

        int ret = memory_pool_slice_free(pool, p, &info);
        assert(ret == FREE_RET_Ok);
        set_slice_mem_value(&info, -1);
        check_meta_running_state(pool);
        for (int i = 0; i < info.num; i++) {
            memory_pool_free(pool, info.slice_arr[i].ptr);
        }
        check_meta_complete_state(pool);
    }

    //test 3
    {
        void *p = memory_pool_malloc(pool, 2000);
        assert(p);
        check_meta_running_state(pool);

        slice_info_array info;
        info.num = 2;
        info.slice_arr[0].ptr = (uint8_t *)p + 100;
        info.slice_arr[0].size = 100;
        info.slice_arr[1].ptr = (uint8_t *)p + 1900;
        info.slice_arr[1].size = 100;

        int ret = memory_pool_slice_free(pool, p, &info);
        assert(ret == FREE_RET_Ok);
        set_slice_mem_value(&info, -1);
        check_meta_running_state(pool);
        for (int i = 0; i < info.num; i++) {
            memory_pool_free(pool, info.slice_arr[i].ptr);
        }
        check_meta_complete_state(pool);
    }

    //test 4
    {
        void *p = memory_pool_malloc(pool, 2000);
        assert(p);
        check_meta_running_state(pool);

        slice_info_array info;
        info.num = SLICE_INFO_NUM_MAX;
        info.slice_arr[0].ptr = (uint8_t *)p + 1500;
        info.slice_arr[0].size = 100;
        info.slice_arr[1].ptr = (uint8_t *)p + 1100;
        info.slice_arr[1].size = 100;
        info.slice_arr[2].ptr = (uint8_t *)p + 500;
        info.slice_arr[2].size = 100;
        info.slice_arr[3].ptr = (uint8_t *)p + 1300;
        info.slice_arr[3].size = 100;
        info.slice_arr[4].ptr = (uint8_t *)p + 900;
        info.slice_arr[4].size = 100;
        info.slice_arr[5].ptr = (uint8_t *)p + 300;
        info.slice_arr[5].size = 100;
        info.slice_arr[6].ptr = (uint8_t *)p + 700;
        info.slice_arr[6].size = 100;
        info.slice_arr[7].ptr = (uint8_t *)p + 100;
        info.slice_arr[7].size = 100;

        int ret = memory_pool_slice_free(pool, p, &info);
        assert(ret == FREE_RET_Ok);
        set_slice_mem_value(&info, -1);
        check_meta_running_state(pool);
        for (int i = 0; i < info.num; i++) {
            memory_pool_free(pool, info.slice_arr[i].ptr);
        }
        check_meta_complete_state(pool);
    }

    //end
    memory_pool_destroy(pool);
}

bool quit = false;
#define THREADS_NUM 100
#define MEM_1M (1024 * 1024)
#define TEST_TIME 3000

void test_thread_free(memory_pool *pool)
{
    int size = rand() % MEM_1M + 1;

    void *p = memory_pool_malloc(pool, size);
    assert(p);

#ifdef _WIN32
    Sleep((rand() % 1000) + 200);
#else
    //TODO

#endif

    memory_pool_free(pool, p);
}

void test_thread_slice_free(memory_pool *pool)
{
    int size = rand() % MEM_1M + MEM_1M;

    void *p = memory_pool_malloc(pool, size);
    assert(p);

#ifdef _WIN32
    Sleep((rand() % 1000) + 200);
#else
    //TODO

#endif

    slice_info_array info;
    memset(&info, 0, sizeof(info));

    info.num = (rand() % (SLICE_INFO_NUM_MAX - 1)) + 1;

    int slice_max_size = size / info.num;
    for (int i = 0; i < info.num; i++) {

        //确保偏移后留够可用于保存元数据的空间
        int slice_offset = (rand() % slice_max_size) - (SLICE_SIZE_MIN + 1);
        if (slice_offset < 0) {
            slice_offset = 0;
        }
        int slice_size = (rand() % (slice_max_size - slice_offset)) + (SLICE_SIZE_MIN + 1);

        info.slice_arr[i].ptr = (uint8_t *)p + i * slice_max_size + slice_offset;
        info.slice_arr[i].size = slice_size;
    }

    slice_info_array old_info;
    memcpy(&old_info, &info, sizeof(slice_info_array));

    int ret = memory_pool_slice_free(pool, p, &info);
    assert(ret == FREE_RET_Ok || ret == FREE_RET_Not_In_Memory_Pool);

    if (ret == FREE_RET_Ok) {
        set_slice_mem_value(&info, -1);
        for (int i = 0; i < info.num; i++) {
            memory_pool_free(pool, info.slice_arr[i].ptr);
        }
    }
    else if (ret == FREE_RET_Not_In_Memory_Pool) {
        os_free(p);
    }
}

void test_multi_thread_i(memory_pool *pool)
{
    do
    {
        test_thread_free(pool);
        test_thread_slice_free(pool);

    } while (!quit);
}

void test_multi_thread()
{

    memory_pool *pool = memory_pool_create(2 * THREADS_NUM * MEM_1M, true);
    assert(pool);
    check_meta_complete_state(pool);

    std::thread threads[THREADS_NUM];

    for (int i = 0; i < THREADS_NUM; i++)
    {
        threads[i] = std::thread(test_multi_thread_i, pool);
    }

    printf("%d threads ready to race...\n", THREADS_NUM);

    for (int i = TEST_TIME; i > 0; i--)
    {
        printf("Waiting for %d secondes\n", i);

#ifdef _WIN32
        Sleep(1000);
#else
        //TOOD
#endif

        check_meta_running_state(pool);
    }

    quit = true;

    printf("Waiting child thread exit...\n");

    for (auto & th : threads)
    {
        th.join();
    }

    check_meta_complete_state(pool);
    memory_pool_destroy(pool);
}