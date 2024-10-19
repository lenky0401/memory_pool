# memory_pool
准备实现一个内存池，最大的特点是支持分片释放。

何谓分片释放，比如一次申请了100字节，第一次释放时可以先只释放前50字节，此时app可以继续使用未释放的剩余50字节，等到最后，app可以再次调用释放接口释放剩余的50字节。

# 分片释放API

```c
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
```