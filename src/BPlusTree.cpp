#include "../include/BPlusTree.h"
#include <cstring>
#include <algorithm>
#include <vector>

BPlusTreeNode::Header* BPlusTreeNode::header(Page& page){
    return reinterpret_cast<Header*>(page.buffer);
}

const BPlusTreeNode::Header* BPlusTreeNode::header(const Page& page){
    return reinterpret_cast<const Header*>(page.buffer);
}

int BPlusTree::get_parent_page_id(const Page& page){
    const BPlusTreeNode::Header* h = reinterpret_cast<const BPlusTreeNode::Header*>(page.buffer);
    return h->parent_page_id;
}

void BPlusTree::set_parent_page_id(Page& page,int parent){
    BPlusTreeNode::Header* h = reinterpret_cast<BPlusTreeNode::Header*>(page.buffer);
    h->parent_page_id = parent;
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

int BPlusTree::get_root_page_id() const{
    return root_page_id;
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

bool BPlusTree::searchRange(int start_key, int end_key, std::vector<RID>& rids){
    if(start_key > end_key)
        return false;

    int leaf_page_id = -1;
    Page* leaf = find_leaf(start_key, leaf_page_id);

    if(leaf == nullptr)
        return false;

    bool found_any = false;

    while(leaf != nullptr){
        uint16_t count = BPlusTreeNode::key_count(*leaf);
        int next_page = leaf->next_page;
        bool should_continue = next_page != -1;

        for(uint16_t i = 0; i < count; i++){
            BPlusTreeNode::LeafEntry entry;

            if(!BPlusTreeNode::get_leaf_entry(*leaf, i, entry)){
                buffer.unpinPage(leaf_page_id, false);
                return found_any;
            }

            if(entry.key > end_key){
                buffer.unpinPage(leaf_page_id, false);
                return found_any;
            }

            if(entry.key >= start_key){
                rids.push_back(entry.rid);
                found_any = true;
            }
        }

        buffer.unpinPage(leaf_page_id, false);

        if(!should_continue)
            break;

        leaf_page_id = next_page;
        leaf = buffer.fetchPage(leaf_page_id);

        if(leaf == nullptr)
            break;
    }

    return found_any;
}

Page* BPlusTree::find_leaf(int key, int& leaf_page_id){

    leaf_page_id = root_page_id;
    while(true){

        Page* current = buffer.fetchPage(leaf_page_id);

        if(current == nullptr)
            return nullptr;

        if(!BPlusTreeNode::validate(*current)){
            buffer.unpinPage(leaf_page_id,false);
            return nullptr;
        }

        if(BPlusTreeNode::is_leaf(*current)){
            return current;
        }

        int next = choose_child(*current,key);

        buffer.unpinPage(leaf_page_id,false);

        if(next==-1)
            return nullptr;

        leaf_page_id=next;
    }
}

bool BPlusTree::split_internal(int internal_page,int left_page,int promoted_key,int right_page){
    Page* old_node = buffer.fetchPage(internal_page);

    if(old_node == nullptr)
        return false;

    uint16_t count = BPlusTreeNode::key_count(*old_node);

    std::vector<int> keys;
    std::vector<int> children;

    for(uint16_t i = 0; i < count; i++){
        int k;
        BPlusTreeNode::get_internal_key(*old_node,i,k);
        keys.push_back(k);
    }

    for(uint16_t i = 0; i < count + 1; i++){
        int c;
        BPlusTreeNode::get_internal_child(*old_node,i,c);
        children.push_back(c);
    }

    int pos = 0;

    while(pos < children.size()){
        if(children[pos] == left_page)
            break;
        pos++;
    }

    if(pos == children.size()){
        buffer.unpinPage(internal_page,false);
        return false;
    }

    if(pos > count){
        buffer.unpinPage(internal_page,false);
        return false;
    }

    keys.insert( keys.begin() + pos,promoted_key);

    children.insert(children.begin() + pos + 1,right_page );

    count++;

    int middle = keys.size()/2;

    int up_key = keys[middle];

    int new_page = buffer.allocatePage();

    Page* right = buffer.fetchPage(new_page);

    if(right == nullptr){
        buffer.unpinPage(internal_page,false);
        return false;
    }

    BPlusTreeNode::init_internal(*right, new_page, get_parent_page_id(*old_node));

    BPlusTreeNode::set_key_count(*old_node,0);
    BPlusTreeNode::set_key_count(*right,0);

    // izquierda

    for(int i=0;i<middle;i++){
        BPlusTreeNode::set_internal_key(*old_node,i,keys[i]);
        BPlusTreeNode::set_internal_child(*old_node,i,children[i]);
    }

    BPlusTreeNode::set_internal_child(*old_node, middle,children[middle]);

    BPlusTreeNode::set_key_count(   *old_node, middle );
    // derecha

    int j=0;

    for(int i = middle + 1; i < keys.size(); i++){ 
        BPlusTreeNode::set_internal_key( *right, j,  keys[i]);
        BPlusTreeNode::set_internal_child(*right, j,children[i] ); j++;
    }
    BPlusTreeNode::set_internal_child( *right,  j, children[count] );

    BPlusTreeNode::set_key_count(   *right,  j  );

    // Actualizar padres

    for(int i=0;i<=j;i++){
        int child;
        BPlusTreeNode::get_internal_child( *right,  i,  child);
        Page* p = buffer.fetchPage(child);
        if(p == nullptr){
            buffer.unpinPage(internal_page,false);
            buffer.unpinPage(new_page,false);
            return false;
        }

        set_parent_page_id(*p,new_page);

        buffer.unpinPage(child,true);
    }

    buffer.unpinPage(internal_page,true);
    buffer.unpinPage(new_page,true);

    buffer.flushPage(internal_page);
    buffer.flushPage(new_page);

    return insert_into_parent(internal_page,up_key,new_page);
}

bool BPlusTree::insert_into_leaf(Page& leaf, int key, RID rid)
{
    if(!BPlusTreeNode::is_leaf(leaf))
        return false;

    uint16_t count = BPlusTreeNode::key_count(leaf);

    if(count >= BPlusTreeNode::max_leaf_keys())
        return false;

    std::vector<BPlusTreeNode::LeafEntry> entries;

    BPlusTreeNode::LeafEntry e;

    for(uint16_t i = 0; i < count; i++){
        BPlusTreeNode::get_leaf_entry(leaf, i, e);

        // No permitir claves repetidas
        if(e.key == key)
            return false;

        entries.push_back(e);
    }

    entries.push_back({key, rid});

    std::sort(entries.begin(),entries.end(), [](const auto& a,const auto& b) {
        return a.key < b.key;}
    );

    BPlusTreeNode::set_key_count(leaf,0);

    for(uint16_t i=0;i<entries.size();i++) {
        BPlusTreeNode::set_leaf_entry(
            leaf,
            i,
            entries[i].key,
            entries[i].rid
        );
    }

    BPlusTreeNode::set_key_count(
        leaf,
        static_cast<uint16_t>(entries.size())
    );

    return true;
}


bool BPlusTree::insert_into_parent(int left_page,int promoted_key, int right_page){
    // Obtener la página izquierda
    Page* left = buffer.fetchPage(left_page);

    if(left == nullptr)
        return false;

    int parent_id = get_parent_page_id(*left);

    buffer.unpinPage(left_page,false);

    // Caso especial:
    // la hoja era la raíz
    if(parent_id == -1){
        int new_root_id = buffer.allocatePage();

        Page* root = buffer.fetchPage(new_root_id);

        if(root == nullptr)
            return false;

        BPlusTreeNode::init_internal(*root, new_root_id, -1);

        BPlusTreeNode::set_internal_child(*root, 0, left_page);
        BPlusTreeNode::set_internal_key(*root, 0, promoted_key);
        BPlusTreeNode::set_internal_child(*root, 1, right_page);

        set_parent_page_id(*root, -1);

        Page* l = buffer.fetchPage(left_page);
        Page* r = buffer.fetchPage(right_page);

        if(l == nullptr || r == nullptr)
        {
            if(l) buffer.unpinPage(left_page, false);
            if(r) buffer.unpinPage(right_page, false);
            buffer.unpinPage(new_root_id, false);
            return false;
        }

        set_parent_page_id(*l, new_root_id);
        set_parent_page_id(*r, new_root_id);

        root_page_id = new_root_id;

        buffer.unpinPage(left_page, true);
        buffer.unpinPage(right_page, true);
        buffer.unpinPage(new_root_id, true);

        buffer.flushPage(left_page);
        buffer.flushPage(right_page);
        buffer.flushPage(new_root_id);

        return true;
    }

    // Insertar en un padre existente

    Page* parent = buffer.fetchPage(parent_id);

    if(parent == nullptr)
        return false;

    uint16_t count =
        BPlusTreeNode::key_count(*parent);

    // ¿Hay espacio?

    if(count < BPlusTreeNode::max_internal_keys())
    {
        // Encontrar posición de left_page

        int pos = 0;
        int child;

        while(pos <= count)
        {
            BPlusTreeNode::get_internal_child(
                *parent,
                pos,
                child
            );

            if(child == left_page)
                break;

            pos++;
        }
        // Mover claves

        for(int i=count;i>pos;i--) {
            int k;

            BPlusTreeNode::get_internal_key(
                *parent,
                i-1,
                k
            );

            BPlusTreeNode::set_internal_key(
                *parent,
                i,
                k
            );
        }
        // Mover hijos

        for(int i=count+1;i>pos+1;i--){
            int c;

            BPlusTreeNode::get_internal_child(
                *parent,
                i-1,
                c
            );

            BPlusTreeNode::set_internal_child(
                *parent,
                i,
                c
            );
        }

        // Insertar nueva clave

        BPlusTreeNode::set_internal_key( *parent, pos, promoted_key );

        BPlusTreeNode::set_internal_child(*parent, pos+1, right_page);

        BPlusTreeNode::set_key_count( *parent, count+1);

        // Actualizar padre del hijo derecho

        Page* right = buffer.fetchPage(right_page);

        if(right == nullptr){
            buffer.unpinPage(parent_id, true); // el padre ya quedó modificado en memoria
            buffer.flushPage(parent_id);
            return false;
        }

        set_parent_page_id(*right, parent_id);

        buffer.unpinPage(right_page, true);

        buffer.unpinPage(parent_id, true);

        buffer.flushPage(parent_id);

        return true;
    }

    // Padre lleno

    buffer.unpinPage(parent_id,false);

    return split_internal(parent_id,left_page,promoted_key,right_page);
}


bool BPlusTree::split_leaf(int leaf_page_id, int key, RID rid){
    Page* old_leaf = buffer.fetchPage(leaf_page_id);

    if(old_leaf == nullptr)
        return false;

    // 1. Copiar todas las entradas

    std::vector<BPlusTreeNode::LeafEntry> entries;

    BPlusTreeNode::LeafEntry e;

    uint16_t count = BPlusTreeNode::key_count(*old_leaf);

    for(uint16_t i=0;i<count;i++){
        BPlusTreeNode::get_leaf_entry(*old_leaf,i,e);
        entries.push_back(e);
    }

    // 2. Agregar la nueva clave
    entries.push_back({key,rid});

    std::sort(
        entries.begin(),
        entries.end(),
        [](const auto& a,const auto& b)
        {
            return a.key<b.key;
        }
    );
    
    // 3. Crear nueva hoja

    int new_leaf_id = buffer.allocatePage();

    Page* new_leaf = buffer.fetchPage(new_leaf_id);

    if(new_leaf==nullptr){
        buffer.unpinPage(leaf_page_id,false);
        return false;
    }

    BPlusTreeNode::init_leaf(*new_leaf,new_leaf_id,old_leaf->next_page,get_parent_page_id(*old_leaf));

    old_leaf->next_page = new_leaf_id;

    // 4. Vaciar ambas hojas

    BPlusTreeNode::set_key_count(*old_leaf,0);
    BPlusTreeNode::set_key_count(*new_leaf,0);

    // 5. Repartir

    int middle = entries.size()/2;

    for(int i=0;i<middle;i++){
        BPlusTreeNode::set_leaf_entry(
            *old_leaf,
            i,
            entries[i].key,
            entries[i].rid
        );
    }

    BPlusTreeNode::set_key_count(*old_leaf,middle );

    int j=0;

    for(int i=middle;i<entries.size();i++){
        BPlusTreeNode::set_leaf_entry(
            *new_leaf,
            j,
            entries[i].key,
            entries[i].rid
        );
        j++;
    }

    BPlusTreeNode::set_key_count(
        *new_leaf,
        j
    );

    // 6. Promover

    int promoted = entries[middle].key;
    // 7. Guardar

    buffer.unpinPage( leaf_page_id,  true);

    buffer.unpinPage( new_leaf_id,  true);

    buffer.flushPage(leaf_page_id);
    buffer.flushPage(new_leaf_id);
    // 8. Insertar en el padre
    return insert_into_parent(leaf_page_id,promoted,new_leaf_id);
}

bool BPlusTree::insert(int key, RID rid){

    int leaf_page_id;
    Page* leaf = find_leaf(key, leaf_page_id);

    if(leaf == nullptr)
        return false;

    uint16_t count = BPlusTreeNode::key_count(*leaf);

    // --- Verificar duplicados ANTES de decidir insertar o dividir ---
    BPlusTreeNode::LeafEntry existing;
    for(uint16_t i = 0; i < count; i++){
        if(BPlusTreeNode::get_leaf_entry(*leaf, i, existing) && existing.key == key){
            buffer.unpinPage(leaf_page_id, false);
            return false; // clave ya existe, no se permite duplicado
        }
    }
    if(count < BPlusTreeNode::max_leaf_keys()){
        bool ok = insert_into_leaf(*leaf, key, rid);
        buffer.unpinPage(leaf_page_id, ok);
        if(ok)
            buffer.flushPage(leaf_page_id);
        return ok;
    }
    buffer.unpinPage(leaf_page_id, false);

    return split_leaf(leaf_page_id, key, rid);
}

bool BPlusTree::remove_from_leaf(Page& leaf, int key){
    if(!BPlusTreeNode::is_leaf(leaf))
        return false;

    uint16_t count = BPlusTreeNode::key_count(leaf);
    std::vector<BPlusTreeNode::LeafEntry> entries;
    bool found = false;

    for(uint16_t i = 0; i < count; i++){
        BPlusTreeNode::LeafEntry entry;

        if(!BPlusTreeNode::get_leaf_entry(leaf, i, entry))
            return false;

        if(entry.key == key){
            found = true;
            continue;
        }

        entries.push_back(entry);
    }

    if(!found)
        return false;

    BPlusTreeNode::set_key_count(leaf, 0);

    for(uint16_t i = 0; i < entries.size(); i++){
        BPlusTreeNode::set_leaf_entry(
            leaf,
            i,
            entries[i].key,
            entries[i].rid
        );
    }

    BPlusTreeNode::set_key_count(
        leaf,
        static_cast<uint16_t>(entries.size())
    );

    return true;
}

bool BPlusTree::remove_child_from_parent(int parent_page, int child_page){
    Page* parent = buffer.fetchPage(parent_page);

    if(parent == nullptr)
        return false;

    if(BPlusTreeNode::is_leaf(*parent)){
        buffer.unpinPage(parent_page, false);
        return false;
    }

    uint16_t count = BPlusTreeNode::key_count(*parent);
    int child_index = -1;

    for(uint16_t i = 0; i <= count; i++){
        int current_child = -1;
        BPlusTreeNode::get_internal_child(*parent, i, current_child);

        if(current_child == child_page){
            child_index = i;
            break;
        }
    }

    if(child_index <= 0){
        buffer.unpinPage(parent_page, false);
        return false;
    }

    int key_to_remove = child_index - 1;

    for(uint16_t i = key_to_remove; i + 1 < count; i++){
        int key = 0;
        BPlusTreeNode::get_internal_key(*parent, i + 1, key);
        BPlusTreeNode::set_internal_key(*parent, i, key);
    }

    for(uint16_t i = child_index; i < count; i++){
        int child = -1;
        BPlusTreeNode::get_internal_child(*parent, i + 1, child);
        BPlusTreeNode::set_internal_child(*parent, i, child);
    }

    BPlusTreeNode::set_key_count(*parent, count - 1);

    if(parent_page == root_page_id && count - 1 == 0){
        int only_child = -1;
        BPlusTreeNode::get_internal_child(*parent, 0, only_child);

        Page* child = buffer.fetchPage(only_child);

        if(child == nullptr){
            buffer.unpinPage(parent_page, true);
            buffer.flushPage(parent_page);
            return false;
        }

        set_parent_page_id(*child, -1);
        root_page_id = only_child;

        buffer.unpinPage(only_child, true);
        buffer.flushPage(only_child);
    }

    buffer.unpinPage(parent_page, true);
    buffer.flushPage(parent_page);

    return true;
}

bool BPlusTree::merge_leaf_with_right(int left_leaf_page, int right_leaf_page, int parent_page){
    Page* left = buffer.fetchPage(left_leaf_page);
    Page* right = buffer.fetchPage(right_leaf_page);

    if(left == nullptr || right == nullptr){
        if(left != nullptr)
            buffer.unpinPage(left_leaf_page, false);

        if(right != nullptr)
            buffer.unpinPage(right_leaf_page, false);

        return false;
    }

    if(!BPlusTreeNode::is_leaf(*left) || !BPlusTreeNode::is_leaf(*right)){
        buffer.unpinPage(left_leaf_page, false);
        buffer.unpinPage(right_leaf_page, false);
        return false;
    }

    if(get_parent_page_id(*left) != parent_page || get_parent_page_id(*right) != parent_page){
        buffer.unpinPage(left_leaf_page, false);
        buffer.unpinPage(right_leaf_page, false);
        return false;
    }

    uint16_t left_count = BPlusTreeNode::key_count(*left);
    uint16_t right_count = BPlusTreeNode::key_count(*right);

    if(left_count + right_count > BPlusTreeNode::max_leaf_keys()){
        buffer.unpinPage(left_leaf_page, false);
        buffer.unpinPage(right_leaf_page, false);
        return false;
    }

    for(uint16_t i = 0; i < right_count; i++){
        BPlusTreeNode::LeafEntry entry;
        BPlusTreeNode::get_leaf_entry(*right, i, entry);
        BPlusTreeNode::set_leaf_entry(
            *left,
            left_count + i,
            entry.key,
            entry.rid
        );
    }

    BPlusTreeNode::set_key_count(*left, left_count + right_count);
    left->next_page = right->next_page;

    buffer.unpinPage(left_leaf_page, true);
    buffer.unpinPage(right_leaf_page, false);

    buffer.flushPage(left_leaf_page);

    return remove_child_from_parent(parent_page, right_leaf_page);
}

bool BPlusTree::remove(int key){
    int leaf_page_id = -1;
    Page* leaf = find_leaf(key, leaf_page_id);

    if(leaf == nullptr)
        return false;

    int parent_page = get_parent_page_id(*leaf);
    int right_leaf_page = leaf->next_page;

    bool removed = remove_from_leaf(*leaf, key);

    if(!removed){
        buffer.unpinPage(leaf_page_id, false);
        return false;
    }

    uint16_t new_count = BPlusTreeNode::key_count(*leaf);
    uint16_t min_leaf_keys = BPlusTreeNode::max_leaf_keys() / 2;
    bool needs_merge = leaf_page_id != root_page_id &&
                       parent_page != -1 &&
                       right_leaf_page != -1 &&
                       new_count < min_leaf_keys;

    buffer.unpinPage(leaf_page_id, true);
    buffer.flushPage(leaf_page_id);

    if(!needs_merge)
        return true;

    Page* right = buffer.fetchPage(right_leaf_page);

    if(right == nullptr)
        return true;

    bool can_merge = BPlusTreeNode::is_leaf(*right) &&
                     get_parent_page_id(*right) == parent_page &&
                     new_count + BPlusTreeNode::key_count(*right) <= BPlusTreeNode::max_leaf_keys();

    buffer.unpinPage(right_leaf_page, false);

    if(can_merge)
        return merge_leaf_with_right(leaf_page_id, right_leaf_page, parent_page);

    return true;
}
