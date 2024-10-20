
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
    check_meta_running_state(pool);

    memory_pool_free(pool, p);
    check_meta_complete_state(pool);

    void *p1 = memory_pool_malloc(pool, 100);
    assert(p1);
    void *p2 = memory_pool_malloc(pool, 8000);
    assert(p2);
    void *p3 = memory_pool_malloc(pool, 1000);
    assert(p3);
    void *p4 = memory_pool_malloc(pool, 800);
    assert(p4);
    check_meta_running_state(pool);

    void *p5 = memory_pool_malloc(pool, 1000);
    assert(p5);
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
        assert(ret == SLICE_FREE_RET_Ok);
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
        assert(ret == SLICE_FREE_RET_Ok);
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
        assert(ret == SLICE_FREE_RET_Ok);
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
        info.num = SLICE_INFO_MAX_NUM;
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
        assert(ret == SLICE_FREE_RET_Ok);
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
void test_multi_thread_i(memory_pool *pool)
{
    do
    {
        int size = rand() % 10000;

        void *p = memory_pool_malloc(pool, size);
        assert(p);

#ifdef _WIN32
        Sleep((rand() % 1000) + 200);
#else
        //TODO

#endif

        memory_pool_free(pool, p);

    } while (!quit);
}

#define THREADS_NUM 100
void test_multi_thread()
{
    srand((uint32_t)time(NULL));

    memory_pool *pool = memory_pool_create(10 * 1024 * 1024, true);
    assert(pool);
    check_meta_complete_state(pool);

    std::thread threads[THREADS_NUM];

    for (int i = 0; i < THREADS_NUM; i++)
    {
        threads[i] = std::thread(test_multi_thread_i, pool);
    }

    printf("%d threads ready to race...\n", THREADS_NUM);

    for (int i = 10; i > 0; i--)
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

    for (auto & th : threads)
    {
        th.join();
    }

    check_meta_complete_state(pool);
    memory_pool_destroy(pool);
}