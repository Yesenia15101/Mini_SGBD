#ifndef BUFFERMANAGER_H
#define BUFFERMANAGER_H

#include <vector>
#include <unordered_map>

#include "Frame.h"
#include "PageManager.h"

class BufferManager {

private:
    std::vector<Frame> pool;
    std::unordered_map<int,int> page_table;
    PageManager& disk;
    uint64_t clock;

public:
    BufferManager( int pool_size, PageManager& pm );
    Page* fetchPage(int page_id);
    bool unpinPage(int page_id, bool dirty);
    bool flushPage(int page_id);
    int findVictim();
    int allocatePage();
};

#endif
