#ifndef BUFFERMANAGER_H
#define BUFFERMANAGER_H
 
#include <vector>
#include <unordered_map>
#include <string>
#include "Frame.h"
#include "PageManager.h"
 
class BufferManager {
 
private:
    struct HotPageNode{
        int page_id;
        int access_count;
        HotPageNode* left;
        HotPageNode* right;

        HotPageNode(int id);
    };

    std::vector<Frame> pool;
    std::unordered_map<int,int> page_table;
    PageManager& disk;
    uint64_t clock;
 
    uint64_t hit_count;
    uint64_t miss_count;
    HotPageNode* hot_root;

    HotPageNode* rotateRight(HotPageNode* node);
    HotPageNode* rotateLeft(HotPageNode* node);
    HotPageNode* splay(HotPageNode* root, int page_id);
    void recordPageAccess(int page_id);
    int getAccessCount(int page_id) const;
    bool isHotPage(int page_id) const;
    void collectHotPages(HotPageNode* node, std::vector<std::pair<int,int>>& pages) const;
    void destroyHotPages(HotPageNode* node);

public:
    BufferManager( int pool_size, PageManager& pm );
    ~BufferManager();
    Page* fetchPage(int page_id);
    bool unpinPage(int page_id, bool dirty);
    bool flushPage(int page_id);
    int findVictim();
    int findVictimBySplayCold() const;
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
    std::vector<std::pair<int,int>> getHotPages(int limit) const;
    std::string getReplacementReport() const;
};
 
#endif
