
#ifdef _WIN32
#include <Windows.h>
#endif

#include <thread>
#include <assert.h>

#include "unit_test.h"
#include "memory_check.h"
#include "../memory_common.h"

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
    assert(get_align_order(MP_CACHELINE_SIZE / 2) == 0);
    assert(get_align_order(MP_CACHELINE_SIZE) == 0);
    assert(get_align_order(MP_CACHELINE_SIZE * 2) == 1);
    assert(get_align_order(MP_CACHELINE_SIZE * 3) == 2);
    assert(get_align_order(MP_CACHELINE_SIZE * 4) == 2);
    assert(get_align_order(MP_CACHELINE_SIZE * 512) == 9);
    assert(get_align_order(MP_CACHELINE_SIZE * 1024) == 10);
}

void test_malloc()
{
    memory_pool *pool = memory_pool_create(10000, false);
    assert(pool);
    check_meta_complete_state(pool);

    void *p = memory_pool_malloc(pool, 100);
    assert(p);
    memset(p, -1, 100);
    check_meta_running_state(pool);

    memory_pool_free(pool, p);
    check_meta_complete_state(pool);

    void *p1 = memory_pool_malloc(pool, 100);
    assert(p1);
    memset(p1, -1, 100);
    void *p2 = memory_pool_malloc(pool, 8000);
    assert(p2);
    memset(p2, 0, 8000);
    void *p3 = memory_pool_malloc(pool, 1000);
    assert(p3);
    memset(p3, 0, 1000);
    void *p4 = memory_pool_malloc(pool, 800);
    assert(p4);
    memset(p4, 0, 800);
    check_meta_running_state(pool);

    void *p5 = memory_pool_malloc(pool, 1000);
    assert(p5);
    memset(p5, 0, 1000);
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