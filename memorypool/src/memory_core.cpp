
#include "memory_common.h"

#ifdef _POSIX_THREADS

#else
#include <mutex>
#endif

static void seg_free_order_add(memory_pool *pool, seg_item *item)
{
    uint32_t order = get_align_order(get_seg_item_size(item));

    assert(order >= 0 && order <= pool->memory_max_order);
    seg_head *head = &(pool->seg_head_arr[order]);

    item->free_list_next = head->free_list_next;
    head->free_list_next = item;
    item->free_list_next->free_list_prev = item;
    item->free_list_prev = (seg_item *)head;
}

static int32_t in_memory_pool_range(memory_pool *pool, void *ptr, int32_t size)
{
    assert(pool);
    assert(ptr);
    assert(size > 0);
    if ((uint64_t)pool->memory_addr <= (uint64_t)ptr &&
        (uint64_t)ptr + size <= (uint64_t)pool->memory_addr + pool->memory_max_size) {
        return TRUE;
    }
    return FALSE;
}

/**
 * 判断内存块是否在pool内存池内，并且在内存段item内。
 */
static int32_t in_memory_pool_seg_range(memory_pool *pool, seg_item *item, void *ptr, int32_t size)
{
    assert(pool);
    assert(item);
    assert(ptr);
    assert(size > 0);

    //不在内存池
    if ((uint64_t)pool->memory_addr > (uint64_t)ptr ||
        (uint64_t)ptr + size > (uint64_t)pool->memory_addr + pool->memory_max_size) {
        return FALSE;
    }

    //不在内存段
    if ((uint64_t)pool->memory_addr + item->linear_addr_offset_start > (uint64_t)ptr ||
        (uint64_t)ptr + size > (uint64_t)pool->memory_addr + item->linear_addr_offset_end) {
        return FALSE;
    }

    return TRUE;
}

static int32_t is_memory_pool_addr(memory_pool *pool, void *ptr)
{
    return in_memory_pool_range(pool, ptr, 1);
}

memory_pool* memory_pool_create(uint32_t size, int32_t need_thread_safe)
{
    memory_pool *pool = NULL;

    if (size == 0 || size > MP_MAX_SIZE) {
        mp_print("para size is error: %d", size);
        return NULL;
    }

    pool = (memory_pool *)os_malloc_align(sizeof(memory_pool));
    if (!pool) {
        return NULL;
    }
    memset(pool, 0, sizeof(memory_pool));

    pool->need_thread_safe = need_thread_safe;
    if (pool->need_thread_safe) {
#ifdef _POSIX_THREADS
        if (pthread_mutex_init(&pool->mutex, NULL) != 0) {
            goto err;
        }
#endif
    }

    pool->memory_max_size = size;
    pool->memory_addr = (uint8_t *)os_malloc_align(size);
    if (!pool->memory_addr) {
        goto err;
    }

    pool->memory_max_order = get_align_order(size);
    pool->seg_head_arr = (seg_head *)os_malloc_align((pool->memory_max_order + 1)
        * sizeof(seg_head));
    if (!pool->seg_head_arr) {
        goto err;
    }
    for (uint32_t i = 0; i <= pool->memory_max_order; i++) {
        pool->seg_head_arr[i].free_list_prev = 
            pool->seg_head_arr[i].free_list_next = (seg_item *)&(pool->seg_head_arr[i]);
    }

    seg_item *item = (seg_item *)pool->memory_addr;
    item->linear_addr_offset_start = 0;
    item->linear_addr_offset_end = size;
    item->linear_addr_list_next = item->linear_addr_list_prev = item;
    item->state = STAT_IN_FREE;
    item->magic = SEG_ITEM_MAGIC;

    seg_free_order_add(pool, item);

    return pool;

err:
    memory_pool_destroy(pool);
    return NULL;
}

void memory_pool_destroy(memory_pool *pool)
{
    if (!pool) {
        return;
    }

    if (pool->need_thread_safe) {
#ifdef _POSIX_THREADS
        pthread_mutex_destroy(&pool->mutex);
#endif
    }

    if (pool->memory_addr) {
        os_free_align(pool->memory_addr);
    }

    if (pool->seg_head_arr) {
        os_free_align(pool->seg_head_arr);
    }

    os_free_align(pool);
}

void* memory_pool_malloc_inner(memory_pool *pool, uint32_t size)
{
    uint32_t need_size = size + sizeof(seg_item);
    uint32_t need_order = get_align_order(need_size);
    for (uint32_t curt_order = need_order; curt_order <= pool->memory_max_order; curt_order++) {

        seg_head *head = &(pool->seg_head_arr[curt_order]);
        seg_item *item = (seg_item *)head->free_list_next;
        if (item == (seg_item *)head) {
            continue;
        }

        while (item != (seg_item *)head) {

            assert(item->state == STAT_IN_FREE);

            if (get_seg_item_size(item) >= need_size) {
                break;
            }
            item = (seg_item *)item->free_list_next;
        }

        //还是没找到可用空间
        if (item == (seg_item *)head) {
            continue;
        }

        //先从free链表里拆出来
        item->free_list_next->free_list_prev = item->free_list_prev;
        item->free_list_prev->free_list_next = item->free_list_next;

        //如果分配后，该item的剩余空间小于sizeof(seg_item)，则由于无法存储元数据，只能浪费了
        if (get_seg_item_size(item) < need_size + sizeof(seg_item)) {
            item->state = STAT_IN_USE;
            return (item + 1);
        }

        //如果有多的空间，需分出来
        assert(get_seg_item_size(item) > need_size);
        seg_item *ret_item = item;
        item = (seg_item *)((uint8_t *)ret_item + need_size);
        item->linear_addr_offset_start = ret_item->linear_addr_offset_start + need_size;
        item->linear_addr_offset_end = ret_item->linear_addr_offset_end;
        ret_item->linear_addr_offset_end = item->linear_addr_offset_start;

        item->linear_addr_list_next = ret_item->linear_addr_list_next;
        ret_item->linear_addr_list_next = item;
        item->linear_addr_list_prev = ret_item;
        item->linear_addr_list_next->linear_addr_list_prev = item;

        item->state = STAT_IN_FREE;
        item->magic = SEG_ITEM_MAGIC;

        //分出来的空间保存到free链表
        seg_free_order_add(pool, item);

        //返回
        ret_item->state = STAT_IN_USE;
        return (ret_item + 1);
    }

    return NULL;
}

void* memory_pool_malloc(memory_pool *pool, uint32_t size)
{
    if (size == 0) {
        return NULL;
    }
    if (pool == NULL || size > MP_MAX_SIZE) {
        return os_malloc(size);
    }

    void *ret_buf = NULL;

    if (pool->need_thread_safe) {
#ifdef _POSIX_THREADS
        pthread_mutex_lock(&pool->mutex);
#else
        std::lock_guard<std::mutex> guard(pool->mutex);
#endif

        ret_buf = memory_pool_malloc_inner(pool, size);

#ifdef _POSIX_THREADS
        pthread_mutex_unlock(&pool->mutex);
#endif
    }
    else {
        ret_buf = memory_pool_malloc_inner(pool, size);
    }

    return ret_buf ? ret_buf : os_malloc(size);
}

static void memory_pool_free_inner(memory_pool *pool, seg_item *curt)
{
    assert(pool);
    assert(curt);

    seg_item *prev = curt->linear_addr_list_prev;
    seg_item *next = curt->linear_addr_list_next;

    assert(prev->magic == SEG_ITEM_MAGIC);
    assert(prev->state == STAT_IN_FREE || prev->state == STAT_IN_USE);
    assert(prev->linear_addr_offset_end == curt->linear_addr_offset_start ||
        (prev->linear_addr_offset_end == pool->memory_max_size &&
            (curt->linear_addr_offset_start == 0)));

    assert(next->magic == SEG_ITEM_MAGIC);
    assert(next->state == STAT_IN_FREE || next->state == STAT_IN_USE);
    assert(next->linear_addr_offset_start == curt->linear_addr_offset_end ||
        (next->linear_addr_offset_start == 0 &&
            (curt->linear_addr_offset_end == pool->memory_max_size)));

    curt->state = STAT_IN_FREE;

    int32_t prev_merge = prev != curt && prev->state == curt->state &&
        prev->linear_addr_offset_end == curt->linear_addr_offset_start;
    int32_t next_merge = next != curt && next->state == curt->state &&
        next->linear_addr_offset_start == curt->linear_addr_offset_end;

    //与前后块都能合并
    if (prev_merge && next_merge)
    {
        //把上一块的空间拉大
        prev->linear_addr_offset_end = next->linear_addr_offset_end;

        //把当前块和下一块从linear链表删除
        prev->linear_addr_list_next = next->linear_addr_list_next;
        next->linear_addr_list_next->linear_addr_list_prev = prev;

        //把上一块和下一块都从free链表删除
        prev->free_list_prev->free_list_next = prev->free_list_next;
        prev->free_list_next->free_list_prev = prev->free_list_prev;
        next->free_list_prev->free_list_next = next->free_list_next;
        next->free_list_next->free_list_prev = next->free_list_prev;

        //把上一块加入free链表
        seg_free_order_add(pool, prev);
    }

    //与前块能合并
    else if (prev_merge)
    {
        //把上一块的空间拉大
        prev->linear_addr_offset_end = curt->linear_addr_offset_end;

        //把当前块从linear链表删除
        prev->linear_addr_list_next = curt->linear_addr_list_next;
        curt->linear_addr_list_next->linear_addr_list_prev = prev;

        //把上一块从free链表删除
        prev->free_list_prev->free_list_next = prev->free_list_next;
        prev->free_list_next->free_list_prev = prev->free_list_prev;

        //把上一块加入free链表
        seg_free_order_add(pool, prev);
    }

    //与后块能合并
    else if (next_merge)
    {
        //把当前块的空间拉大
        curt->linear_addr_offset_end = next->linear_addr_offset_end;

        //把下一块从linear链表删除
        curt->linear_addr_list_next = next->linear_addr_list_next;
        next->linear_addr_list_next->linear_addr_list_prev = curt;

        //把下一块从free链表删除
        next->free_list_prev->free_list_next = next->free_list_next;
        next->free_list_next->free_list_prev = next->free_list_prev;

        //把当前块加入free链表
        seg_free_order_add(pool, curt);
    }

    //与前后块都不能合并
    else {
        //把当前块加入linear链表
        prev->linear_addr_list_next = curt;
        curt->linear_addr_list_prev = prev;
        curt->linear_addr_list_next = next;
        next->linear_addr_list_prev = curt;

        //把当前块加入free链表
        seg_free_order_add(pool, curt);
    }
}

int memory_pool_free(memory_pool *pool, void *ptr)
{
    //参数错误
    if (!pool || !ptr) {
        return SLICE_FREE_RET_Bad_Parameter;
    }

    //不在内存池范围里，直接释放
    if (!is_memory_pool_addr(pool, ptr)) {
        os_free(ptr);
        return SLICE_FREE_RET_Ok;
    }

    seg_item *curt = (seg_item *)((uint8_t *)ptr - sizeof(seg_item));
    assert(curt->magic == SEG_ITEM_MAGIC);
    assert(curt->state == STAT_IN_USE);

    if (pool->need_thread_safe) {
#ifdef _POSIX_THREADS
        pthread_mutex_lock(&pool->mutex);
#else
        std::lock_guard<std::mutex> guard(pool->mutex);
#endif

        memory_pool_free_inner(pool, curt);

#ifdef _POSIX_THREADS
        pthread_mutex_unlock(&pool->mutex);
#endif
    }
    else {
        memory_pool_free_inner(pool, curt);
    }

    return SLICE_FREE_RET_Ok;
}

void memory_pool_slice_free_inner(memory_pool *pool, void *ptr, slice_info_array *info)
{
    seg_item *head = (seg_item *)((uint8_t *)ptr - sizeof(seg_item));

    //逐个进行分片内存释放，从大地址往小地址走
    int s_i = 0;
    int ret_i = SLICE_INFO_NUM_MAX;
    seg_item *curt = NULL;

    for (int i = info->num - 1; i >= 0; i--) {
        void* ptr = info->slice_arr[i].ptr;
        int32_t size = info->slice_arr[i].size;

        curt = (seg_item *)((uint8_t *)ptr);
        curt->magic = SEG_ITEM_MAGIC;
        curt->state = STAT_IN_FREE;
        curt->linear_addr_offset_start = (uint32_t)((uint64_t)ptr - (uint64_t)pool->memory_addr);
        curt->linear_addr_offset_end = curt->linear_addr_offset_start + size;

        curt->linear_addr_list_prev = head;
        curt->linear_addr_list_next = head->linear_addr_list_next;
        head->linear_addr_list_next = curt;
        curt->linear_addr_list_next->linear_addr_list_prev = curt;

        assert(head->linear_addr_offset_start <= curt->linear_addr_offset_start);
        assert(head->linear_addr_offset_end >= curt->linear_addr_offset_end);

        //后面还有使用中内存？
        if (head->linear_addr_offset_end > curt->linear_addr_offset_end) {
            seg_item *item = (seg_item *)(pool->memory_addr + curt->linear_addr_offset_end - sizeof(seg_item));
            item->magic = SEG_ITEM_MAGIC;
            item->state = STAT_IN_USE;
            item->linear_addr_offset_start = curt->linear_addr_offset_end - sizeof(seg_item);
            item->linear_addr_offset_end = head->linear_addr_offset_end;
            item->linear_addr_list_prev = curt;
            item->linear_addr_list_next = curt->linear_addr_list_next;
            curt->linear_addr_list_next = item;
            item->linear_addr_list_next->linear_addr_list_prev = item;

            curt->linear_addr_offset_end = item->linear_addr_offset_start;

            info->slice_arr[ret_i].ptr = (uint8_t *)item + sizeof(seg_item);
            info->slice_arr[ret_i].size = item->linear_addr_offset_end -
                item->linear_addr_offset_start - sizeof(seg_item);
            ret_i--;
        }

        head->linear_addr_offset_end = curt->linear_addr_offset_start;

#ifdef __DEBUG
        curt->free_list_prev = (seg_item *)SEG_ITEM_INVALID_VALUE;
        curt->free_list_next = (seg_item *)SEG_ITEM_INVALID_VALUE;
#endif
        if (i > 0) {
            memory_pool_free_inner(pool, curt);
        }
    }

    //最前面还有一块使用中内存
    if (head->linear_addr_offset_start + sizeof(seg_item) < curt->linear_addr_offset_start) {
        info->slice_arr[ret_i].ptr = (uint8_t *)head + sizeof(seg_item);
        info->slice_arr[ret_i].size = head->linear_addr_offset_end -
            head->linear_addr_offset_start - sizeof(seg_item);
        ret_i--;

        memory_pool_free_inner(pool, curt);
    }
    //把head的meta放到free里
    else {
        head->linear_addr_list_next = curt->linear_addr_list_next;
        curt->linear_addr_list_next->linear_addr_list_prev = head;
        head->linear_addr_offset_end = curt->linear_addr_offset_end;

        memory_pool_free_inner(pool, head);
    }

    info->num = SLICE_INFO_NUM_MAX - ret_i;
    if (info->num <= SLICE_INFO_NUM_MAX) {
        int offset = ret_i + 1;
        for (int i = 0; i < info->num; i++) {
            info->slice_arr[i] = info->slice_arr[i + offset];
        }
    }
}

int memory_pool_slice_free(memory_pool *pool, void *ptr, slice_info_array *info)
{
    //参数错误
    if (!pool || !ptr || !info) {
        return SLICE_FREE_RET_Bad_Parameter;
    }

    //不在内存池范围里，不支持分片释放
    if (!is_memory_pool_addr(pool, ptr)) {
        return SLICE_FREE_RET_Not_Supported;
    }

    //参数错误
    //1，分片内存个数是否错误
    if (info->num <= 0 || info->num > SLICE_INFO_NUM_MAX) {
        return SLICE_FREE_RET_Bad_Slice_Info;
    }

    /**
     * 2，分片内存太小或不在ptr内存段范围里
     * 说明：之所以要大于等于两倍SLICE_FREE_MIN_SIZE，是因为其自身需要一个SLICE_FREE_MIN_SIZE，
     * 而其后面的使用中内存也需要一个SLICE_FREE_MIN_SIZE。
     */
    seg_item *head = (seg_item *)((uint8_t *)ptr - sizeof(seg_item));
    assert(head->magic == SEG_ITEM_MAGIC);
    assert(head->state == STAT_IN_USE);
    for (int i = 0; i < info->num; i++) {
        void* ptr = info->slice_arr[i].ptr;
        int32_t size = info->slice_arr[i].size;
        if (size < SLICE_SIZE_MIN || !in_memory_pool_seg_range(pool, head, ptr, size)) {
            return SLICE_FREE_RET_Bad_Slice_Info;
        }
    }
    //3，分片内存之间是否有相互重叠
    for (int i = 0; i < info->num; i++) {
        uint64_t i_start = (uint64_t)info->slice_arr[i].ptr;
        uint64_t i_end = i_start + info->slice_arr[i].size;
        for (int j = i + 1; j < info->num; j++) {
            uint64_t j_start = (uint64_t)info->slice_arr[j].ptr;
            uint64_t j_end = j_start + info->slice_arr[j].size;
            /**
             * 不重叠的情况：
             * 1，线段1的终点在线段2的始点之前，或
             * 2，线段2的终点在线段1的始点之前。
             * 相反情况则是有重叠，有重叠则返回错误。
             */
            if (!(i_end <= j_start || j_end <= i_start)) {
                return SLICE_FREE_RET_Bad_Slice_Info;
            }
        }
    }

    //对待释放的分片内存按地址做排序，大的地址放后面
    slice_info tmp;
    for (int i = 0; i < info->num; i++) {
        for (int j = 1; j < info->num - i; j++) {
            if ((uint64_t)info->slice_arr[j - 1].ptr >
                (uint64_t)info->slice_arr[j].ptr)
            {
                tmp = info->slice_arr[j - 1];
                info->slice_arr[j - 1] = info->slice_arr[j];
                info->slice_arr[j] = tmp;
            }
        }
    }

    if (pool->need_thread_safe) {
#ifdef _POSIX_THREADS
        pthread_mutex_lock(&pool->mutex);
#else
        std::lock_guard<std::mutex> guard(pool->mutex);
#endif

        memory_pool_slice_free_inner(pool, ptr, info);

#ifdef _POSIX_THREADS
        pthread_mutex_unlock(&pool->mutex);
#endif
    }
    else {
        memory_pool_slice_free_inner(pool, ptr, info);
    }

    return SLICE_FREE_RET_Ok;
}