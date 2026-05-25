#include "../include/BufferManager.h"

BufferManager::BufferManager( int pool_size, PageManager& pm ): disk(pm), clock(0) {
    pool.resize(pool_size);
}

Page* BufferManager::fetchPage(int page_id) {
    if (page_table.count(page_id)) {
        int frame_index = page_table[page_id];
        Frame& frame = pool[frame_index];
        frame.last_used = clock++;
        return &frame.page;
    }
    for (int i = 0; i < pool.size(); i++){
        if (!pool[i].occupied) {
            pool[i].page = disk.read_page(page_id);
            pool[i].occupied = true;
            pool[i].page_id = page_id;
            pool[i].last_used = clock++;
            page_table[page_id] = i;
            return &pool[i].page;
        }
    }
    return nullptr;
}