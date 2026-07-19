#include "../include/BufferManager.h"
#include <algorithm>
#include <climits>
#include <sstream>

const int HOT_PAGE_THRESHOLD = 10;

BufferManager::HotPageNode::HotPageNode(int id):
    page_id(id),
    access_count(1),
    left(nullptr),
    right(nullptr) {
}

BufferManager::BufferManager( int pool_size, PageManager& pm )
    : disk(pm), clock(0), hit_count(0), miss_count(0), hot_root(nullptr) {
    pool.resize(pool_size);
}

BufferManager::~BufferManager() {
    destroyHotPages(hot_root);
}

BufferManager::HotPageNode* BufferManager::rotateRight(HotPageNode* node) {
    HotPageNode* left = node->left;
    node->left = left->right;
    left->right = node;
    return left;
}

BufferManager::HotPageNode* BufferManager::rotateLeft(HotPageNode* node) {
    HotPageNode* right = node->right;
    node->right = right->left;
    right->left = node;
    return right;
}

BufferManager::HotPageNode* BufferManager::splay(HotPageNode* root, int page_id) {
    if(root == nullptr || root->page_id == page_id)
        return root;

    if(page_id < root->page_id) {
        if(root->left == nullptr)
            return root;

        if(page_id < root->left->page_id) {
            root->left->left = splay(root->left->left, page_id);
            root = rotateRight(root);
        } else if(page_id > root->left->page_id) {
            root->left->right = splay(root->left->right, page_id);

            if(root->left->right != nullptr)
                root->left = rotateLeft(root->left);
        }

        if(root->left == nullptr)
            return root;

        return rotateRight(root);
    }

    if(root->right == nullptr)
        return root;

    if(page_id > root->right->page_id) {
        root->right->right = splay(root->right->right, page_id);
        root = rotateLeft(root);
    } else if(page_id < root->right->page_id) {
        root->right->left = splay(root->right->left, page_id);

        if(root->right->left != nullptr)
            root->right = rotateRight(root->right);
    }

    if(root->right == nullptr)
        return root;

    return rotateLeft(root);
}

void BufferManager::recordPageAccess(int page_id) {
    if(hot_root == nullptr) {
        hot_root = new HotPageNode(page_id);
        return;
    }

    hot_root = splay(hot_root, page_id);

    if(hot_root->page_id == page_id) {
        hot_root->access_count++;
        return;
    }

    HotPageNode* node = new HotPageNode(page_id);

    if(page_id < hot_root->page_id) {
        node->right = hot_root;
        node->left = hot_root->left;
        hot_root->left = nullptr;
    } else {
        node->left = hot_root;
        node->right = hot_root->right;
        hot_root->right = nullptr;
    }

    hot_root = node;
}

int BufferManager::getAccessCount(int page_id) const {
    HotPageNode* current = hot_root;

    while(current != nullptr) {
        if(current->page_id == page_id)
            return current->access_count;

        if(page_id < current->page_id)
            current = current->left;
        else
            current = current->right;
    }

    return 0;
}

bool BufferManager::isHotPage(int page_id) const {
    return getAccessCount(page_id) >= HOT_PAGE_THRESHOLD;
}

void BufferManager::collectHotPages(HotPageNode* node, std::vector<std::pair<int,int>>& pages) const {
    if(node == nullptr)
        return;

    collectHotPages(node->left, pages);
    pages.push_back({node->page_id, node->access_count});
    collectHotPages(node->right, pages);
}

void BufferManager::destroyHotPages(HotPageNode* node) {
    if(node == nullptr)
        return;

    destroyHotPages(node->left);
    destroyHotPages(node->right);
    delete node;
}

std::vector<std::pair<int,int>> BufferManager::getHotPages(int limit) const {
    std::vector<std::pair<int,int>> pages;
    collectHotPages(hot_root, pages);

    std::sort(
        pages.begin(),
        pages.end(),
        [](const auto& a, const auto& b) {
            if(a.second == b.second)
                return a.first < b.first;

            return a.second > b.second;
        }
    );

    if(limit >= 0 && static_cast<int>(pages.size()) > limit)
        pages.resize(limit);

    return pages;
}

int BufferManager::findVictimBySplayCold() const {
    int victim = -1;
    int lowest_access_count = INT_MAX;
    uint64_t oldest = UINT64_MAX;

    for(int i = 0; i < pool.size(); i++) {
        if(!pool[i].occupied || pool[i].pin_count > 0)
            continue;

        int accesses = getAccessCount(pool[i].page_id);

        if(accesses < lowest_access_count ||
           (accesses == lowest_access_count && pool[i].last_used < oldest)) {
            lowest_access_count = accesses;
            oldest = pool[i].last_used;
            victim = i;
        }
    }

    return victim;
}

std::string BufferManager::getReplacementReport() const {
    int lru_frame = -1;
    uint64_t oldest = UINT64_MAX;

    for(int i = 0; i < pool.size(); i++) {
        if(!pool[i].occupied || pool[i].pin_count > 0)
            continue;

        if(pool[i].last_used < oldest) {
            oldest = pool[i].last_used;
            lru_frame = i;
        }
    }

    std::ostringstream out;
    out << "Decision del Buffer Pool\n";
    out << "   Politica base: LRU\n";
    out << "   Umbral de pagina caliente: " << HOT_PAGE_THRESHOLD << " accesos\n";

    if(lru_frame == -1) {
        out << "   No hay paginas candidatas: todas estan pineadas o el buffer esta vacio.\n";
        return out.str();
    }

    out << "   Victima segun LRU puro: pagina " << pool[lru_frame].page_id
        << " (" << getAccessCount(pool[lru_frame].page_id) << " accesos)\n";

    int splay_frame = findVictimBySplayCold();

    if(splay_frame != -1) {
        out << "   Victima segun Splay/pagina fria: pagina " << pool[splay_frame].page_id
            << " (" << getAccessCount(pool[splay_frame].page_id) << " accesos)\n";

        if(pool[lru_frame].page_id != pool[splay_frame].page_id) {
            out << "   Comparacion: las politicas elegirian paginas distintas.\n";
            out << "   LRU mira antiguedad; Splay/pagina fria mira frecuencia de uso.\n";
        } else {
            out << "   Comparacion: ambas politicas elegirian la misma pagina.\n";
        }
    }

    if(isHotPage(pool[lru_frame].page_id)) {
        out << "   Diagnostico Splay: esta pagina es caliente.\n";
        out << "   Si se activara reemplazo por Splay/pagina fria,\n";
        out << "   podria conservar paginas con muchos accesos.\n";
    } else {
        out << "   Diagnostico Splay: esta pagina no supera el umbral caliente.\n";
        out << "   LRU puede reemplazarla sin conflicto.\n";
    }

    return out.str();
}

Page* BufferManager::fetchPage(int page_id) {
    recordPageAccess(page_id);

    if(page_table.count(page_id)) {

        int frame_index = page_table[page_id];

        Frame& frame = pool[frame_index];

        frame.last_used = clock++;
        frame.pin_count++;

        hit_count++; // la pagina ya estaba en memoria: ACIERTO

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

            miss_count++; // hubo que leer de disco: FALLO

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

    miss_count++; // hubo que desalojar y leer de disco: FALLO

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
