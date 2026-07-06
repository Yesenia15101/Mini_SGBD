#include "../include/BufferManager.h"

BufferManager::BufferManager( int pool_size, PageManager& pm ): disk(pm), clock(0) {
    pool.resize(pool_size);
}

Page* BufferManager::fetchPage(int page_id) {
    if(page_table.count(page_id)) {

        int frame_index = page_table[page_id];

        Frame& frame = pool[frame_index];

        frame.last_used = clock++;
        frame.pin_count++;

        return &frame.page;
    }

    for(int i = 0; i < pool.size(); i++) {

        if(!pool[i].occupied) {

            pool[i].page = disk.read_page(page_id);

            pool[i].occupied = true;
            pool[i].page_id = page_id;
            pool[i].last_used = clock++;
            pool[i].pin_count = 1;
            pool[i].dirty = false;

            page_table[page_id] = i;

            return &pool[i].page;
        }
    }

    int victim = findVictim();

    if(victim == -1)
        return nullptr;

    if(pool[victim].dirty) {

        disk.write_page(
            pool[victim].page_id,
            pool[victim].page
        );
    }

    page_table.erase(
        pool[victim].page_id
    );

    pool[victim].page =
        disk.read_page(page_id);

    pool[victim].page_id = page_id;
    pool[victim].last_used = clock++;
    pool[victim].dirty = false;
    pool[victim].pin_count = 1;
    pool[victim].occupied = true;

    page_table[page_id] = victim;

    return &pool[victim].page;
}

bool BufferManager::unpinPage(int page_id, bool dirty) {
    if(!page_table.count(page_id))
        return false;

    Frame& frame = pool[page_table[page_id]];

    if(frame.pin_count > 0)
        frame.pin_count--;

    if(dirty)
        frame.dirty = true;

    return true;
}

bool BufferManager::flushPage(int page_id) {
    if(!page_table.count(page_id))
        return false;

    Frame& frame = pool[page_table[page_id]];

    if(!frame.occupied)
        return false;

    if(!disk.write_page(page_id, frame.page))
        return false;

    frame.dirty = false;

    return true;
}

int BufferManager::findVictim() {
    int victim = -1;
    uint64_t oldest = UINT64_MAX;

    for(int i=0;i<pool.size();i++) {

        if(pool[i].pin_count > 0)
            continue;

        if(pool[i].last_used < oldest) {
            oldest = pool[i].last_used;
            victim = i;
        }
    }

    return victim;
}
int BufferManager::allocatePage(){

    int new_page = disk.allocate_page();

    return new_page;
}
