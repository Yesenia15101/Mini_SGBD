#include "../include/Catalog.h"
#include <cstring>

Catalog::CatalogHeader* Catalog::header(Page& page){
    return reinterpret_cast<CatalogHeader*>(page.buffer);
}

Catalog::TableEntry* Catalog::entry(Page& page, int index){
    return reinterpret_cast<TableEntry*>(
        page.buffer + sizeof(CatalogHeader) + index * sizeof(TableEntry)
    );
}

Catalog::Catalog(BufferManager& bm) : buffer(bm) {
    Page* page = buffer.fetchPage(CATALOG_PAGE_ID);

    if(page == nullptr)
        return; // no deberia pasar; sin pool no hay nada que hacer

    CatalogHeader* h = header(*page);

    if(h->magic != MAGIC){
        // primera vez que se usa este archivo: catalogo vacio
        std::memset(page->buffer, 0, sizeof(page->buffer));
        h = header(*page);
        h->magic = MAGIC;
        h->table_count = 0;

        buffer.unpinPage(CATALOG_PAGE_ID, true);
        buffer.flushPage(CATALOG_PAGE_ID);
    } else {
        // catalogo ya existia: no tocamos nada, solo leimos
        buffer.unpinPage(CATALOG_PAGE_ID, false);
    }
}

bool Catalog::get_table(const std::string& name, int& root_page_id, int& first_data_page){
    Page* page = buffer.fetchPage(CATALOG_PAGE_ID);
    if(page == nullptr) return false;

    CatalogHeader* h = header(*page);
    bool found = false;

    for(uint16_t i = 0; i < h->table_count; i++){
        TableEntry* e = entry(*page, i);

        if(e->in_use && name == e->name){
            root_page_id = e->root_page_id;
            first_data_page = e->first_data_page;
            found = true;
            break;
        }
    }

    buffer.unpinPage(CATALOG_PAGE_ID, false);
    return found;
}

bool Catalog::register_table(const std::string& name, int root_page_id, int first_data_page){
    if(name.size() >= sizeof(TableEntry::name))
        return false;

    Page* page = buffer.fetchPage(CATALOG_PAGE_ID);
    if(page == nullptr) return false;

    CatalogHeader* h = header(*page);

    if(h->table_count >= MAX_TABLES){
        buffer.unpinPage(CATALOG_PAGE_ID, false);
        return false;
    }

    TableEntry* e = entry(*page, h->table_count);

    std::memset(e->name, 0, sizeof(e->name));
    std::strncpy(e->name, name.c_str(), sizeof(e->name) - 1);
    e->root_page_id = root_page_id;
    e->first_data_page = first_data_page;
    e->in_use = 1;

    h->table_count++;

    buffer.unpinPage(CATALOG_PAGE_ID, true);
    buffer.flushPage(CATALOG_PAGE_ID);

    return true;
}