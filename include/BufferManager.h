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
 
    uint64_t hit_count;
    uint64_t miss_count;
 
public:
    BufferManager( int pool_size, PageManager& pm );
    Page* fetchPage(int page_id);
    bool unpinPage(int page_id, bool dirty);
    bool flushPage(int page_id);
    int findVictim();
    int allocatePage();
 
    // Metricas de rendimiento (rubrica item 16: "Medicion de rendimiento
    // y hit rate del Buffer Manager")
    uint64_t get_hit_count() const { return hit_count; }
    uint64_t get_miss_count() const { return miss_count; }
    double get_hit_rate() const {
        uint64_t total = hit_count + miss_count;
        return total == 0 ? 0.0 : static_cast<double>(hit_count) / static_cast<double>(total);
    }
    void reset_stats() { hit_count = 0; miss_count = 0; }
};
 
#endif