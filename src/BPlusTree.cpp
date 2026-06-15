#include "../include/BPlusTree.h"
#include <cstring>

BPlusTreeNode::Header* BPlusTreeNode::header(Page& page){
    return reinterpret_cast<Header*>(page.buffer);
}

const BPlusTreeNode::Header* BPlusTreeNode::header(const Page& page){
    return reinterpret_cast<const Header*>(page.buffer);
}

void BPlusTreeNode::init_leaf(Page& page, int page_id, int next_leaf_page_id, int parent_page_id){
    std::memset(&page, 0, sizeof(Page));

    page.page_id = page_id;
    page.next_page = next_leaf_page_id;

    Header* h = header(page);
    h->magic = MAGIC;
    h->is_leaf = 1;
    h->key_count = 0;
    h->parent_page_id = parent_page_id;
}

void BPlusTreeNode::init_internal(Page& page, int page_id, int parent_page_id){
    std::memset(&page, 0, sizeof(Page));

    page.page_id = page_id;
    page.next_page = -1;

    Header* h = header(page);
    h->magic = MAGIC;
    h->is_leaf = 0;
    h->key_count = 0;
    h->parent_page_id = parent_page_id;
}

bool BPlusTreeNode::is_bplus_node(const Page& page){
    return header(page)->magic == MAGIC;
}

bool BPlusTreeNode::is_leaf(const Page& page){
    return is_bplus_node(page) && header(page)->is_leaf == 1;
}

uint16_t BPlusTreeNode::key_count(const Page& page){
    if(!is_bplus_node(page))
        return 0;

    return header(page)->key_count;
}

uint16_t BPlusTreeNode::max_leaf_keys(){
    return static_cast<uint16_t>((DATA_SIZE - sizeof(Header)) / sizeof(LeafEntry));
}

uint16_t BPlusTreeNode::max_internal_keys(){
    return static_cast<uint16_t>((DATA_SIZE - sizeof(Header) - sizeof(int)) / (sizeof(int) * 2));
}

bool BPlusTreeNode::set_leaf_entry(Page& page, uint16_t index, int key, RID rid){
    if(!is_leaf(page) || index >= max_leaf_keys())
        return false;

    LeafEntry* entries = reinterpret_cast<LeafEntry*>(page.buffer + sizeof(Header));
    entries[index].key = key;
    entries[index].rid = rid;

    Header* h = header(page);
    if(index >= h->key_count)
        h->key_count = index + 1;

    return true;
}

bool BPlusTreeNode::get_leaf_entry(const Page& page, uint16_t index, LeafEntry& entry){
    if(!is_leaf(page) || index >= key_count(page))
        return false;

    const LeafEntry* entries = reinterpret_cast<const LeafEntry*>(page.buffer + sizeof(Header));
    entry = entries[index];

    return true;
}

bool BPlusTreeNode::set_internal_key(Page& page, uint16_t index, int key){
    if(!is_bplus_node(page) || is_leaf(page) || index >= max_internal_keys())
        return false;

    int* keys = reinterpret_cast<int*>(page.buffer + sizeof(Header));
    keys[index] = key;

    Header* h = header(page);
    if(index >= h->key_count)
        h->key_count = index + 1;

    return true;
}

bool BPlusTreeNode::get_internal_key(const Page& page, uint16_t index, int& key){
    if(!is_bplus_node(page) || is_leaf(page) || index >= key_count(page))
        return false;

    const int* keys = reinterpret_cast<const int*>(page.buffer + sizeof(Header));
    key = keys[index];

    return true;
}

bool BPlusTreeNode::set_internal_child(Page& page, uint16_t index, int child_page_id){
    if(!is_bplus_node(page) || is_leaf(page) || index > max_internal_keys())
        return false;

    int* children = reinterpret_cast<int*>(
        page.buffer + sizeof(Header) + max_internal_keys() * sizeof(int)
    );
    children[index] = child_page_id;

    return true;
}

bool BPlusTreeNode::get_internal_child(const Page& page, uint16_t index, int& child_page_id){
    if(!is_bplus_node(page) || is_leaf(page) || index > key_count(page))
        return false;

    const int* children = reinterpret_cast<const int*>(
        page.buffer + sizeof(Header) + max_internal_keys() * sizeof(int)
    );
    child_page_id = children[index];

    return true;
}

bool BPlusTreeNode::set_key_count(Page& page, uint16_t count){
    if(!is_bplus_node(page))
        return false;

    if(is_leaf(page) && count > max_leaf_keys())
        return false;

    if(!is_leaf(page) && count > max_internal_keys())
        return false;

    header(page)->key_count = count;

    return true;
}

bool BPlusTreeNode::validate(const Page& page){
    if(!is_bplus_node(page))
        return false;

    uint16_t count = key_count(page);

    if(is_leaf(page))
        return count <= max_leaf_keys();

    if(count > max_internal_keys())
        return false;

    int child = -1;
    return get_internal_child(page, 0, child);
}

BPlusTree::BPlusTree(BufferManager& bm, int root_page_id): buffer(bm), root_page_id(root_page_id){
}

bool BPlusTree::create_empty_tree(){
    Page* root = buffer.fetchPage(root_page_id);

    if(root == nullptr)
        return false;

    BPlusTreeNode::init_leaf(*root, root_page_id);

    bool unpinned = buffer.unpinPage(root_page_id, true);
    bool flushed = buffer.flushPage(root_page_id);

    return unpinned && flushed;
}

int BPlusTree::choose_child(const Page& internal_node, int key) const{
    uint16_t count = BPlusTreeNode::key_count(internal_node);
    uint16_t child_index = 0;

    while(child_index < count){
        int separator = 0;

        if(!BPlusTreeNode::get_internal_key(internal_node, child_index, separator))
            return -1;

        if(key < separator)
            break;

        child_index++;
    }

    int child_page_id = -1;

    if(!BPlusTreeNode::get_internal_child(internal_node, child_index, child_page_id))
        return -1;

    return child_page_id;
}

bool BPlusTree::search(int key, RID& rid){
    int current_page_id = root_page_id;

    while(true){
        Page* current = buffer.fetchPage(current_page_id);

        if(current == nullptr)
            return false;

        if(!BPlusTreeNode::validate(*current)){
            buffer.unpinPage(current_page_id, false);
            return false;
        }

        if(BPlusTreeNode::is_leaf(*current)){
            uint16_t count = BPlusTreeNode::key_count(*current);

            for(uint16_t i = 0; i < count; i++){
                BPlusTreeNode::LeafEntry entry;

                if(!BPlusTreeNode::get_leaf_entry(*current, i, entry)){
                    buffer.unpinPage(current_page_id, false);
                    return false;
                }

                if(entry.key == key){
                    rid = entry.rid;
                    buffer.unpinPage(current_page_id, false);
                    return true;
                }

                if(entry.key > key)
                    break;
            }

            buffer.unpinPage(current_page_id, false);
            return false;
        }

        int next_page_id = choose_child(*current, key);
        buffer.unpinPage(current_page_id, false);

        if(next_page_id < 0)
            return false;

        current_page_id = next_page_id;
    }
}
