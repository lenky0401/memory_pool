# memory_pool
准备实现一个内存池，支持部分释放。
何谓部分释放，比如申请100字节，释放时可以先只释放50字节，app可以继续使用未使用的50字节，等使用完最后再释放剩余的50字节。