#include "../include/BufferManager.h"

BufferManager::BufferManager(
    int pool_size,
    PageManager& pm
)
    : disk(pm), clock(0)
{
    pool.resize(pool_size);
}