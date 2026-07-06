#ifndef BPLUSTREE_H
#define BPLUSTREE_H

#include <cstdint>
#include "BufferManager.h"
#include "Page.h"

class BPlusTreeNode{
public:
    static const uint16_t MAGIC = 0xB17E;

#pragma pack(push, 1)
    struct Header{
        uint16_t magic;
        uint8_t is_leaf;
        uint8_t reserved;
        uint16_t key_count;
        uint16_t reserved2;
        int parent_page_id;
    };
#pragma pack(pop)

#pragma pack(push, 1)
    struct LeafEntry{
        int key;
        RID rid;
    };
#pragma pack(pop)

    static const uint16_t DATA_SIZE = PAGE_SIZE - (sizeof(int) * 2);

    static void init_leaf(Page& page, int page_id, int next_leaf_page_id = -1, int parent_page_id = -1);
    static void init_internal(Page& page, int page_id, int parent_page_id = -1);

    static bool is_bplus_node(const Page& page);
    static bool is_leaf(const Page& page);
    static uint16_t key_count(const Page& page);
    static uint16_t max_leaf_keys();
    static uint16_t max_internal_keys();

    static bool set_leaf_entry(Page& page, uint16_t index, int key, RID rid);
    static bool get_leaf_entry(const Page& page, uint16_t index, LeafEntry& entry);

    static bool set_internal_key(Page& page, uint16_t index, int key);
    static bool get_internal_key(const Page& page, uint16_t index, int& key);
    static bool set_internal_child(Page& page, uint16_t index, int child_page_id);
    static bool get_internal_child(const Page& page, uint16_t index, int& child_page_id);

    static bool set_key_count(Page& page, uint16_t count);
    static bool validate(const Page& page);

private:
    static Header* header(Page& page);
    static const Header* header(const Page& page);
};

class BPlusTree{
private:
    BufferManager& buffer;
    int root_page_id;

    int choose_child(const Page& internal_node, int key) const;
    Page* find_leaf( int key, int& leaf_page_id);
    bool insert_into_leaf( Page& leaf, int key, RID rid);
    bool split_leaf( int leaf_page_id, int key, RID rid);
    bool insert_into_parent( int left_page,int promoted_key,int right_page);
    bool split_internal(int internal_page,int left_page,int promoted_key,int right_page);

public:
    BPlusTree(BufferManager& bm, int root_page_id);
    static int get_parent_page_id(const Page& page);
    static void set_parent_page_id(Page& page, int parent);

    bool create_empty_tree();
    bool search(int key, RID& rid);
    bool insert(int key,RID rid);
    bool remove(int key);
    
};

#endif
