#include "../include/Table.h"

Table::Table(BufferManager& bm)
    : buffer(bm), index(bm, bm.allocatePage())
{
    // index ya reservo su pagina raiz (index.get_root_page_id()),
    // pero esa pagina llega llena de ceros desde PageManager::allocate_page.
    // Este era el paso que faltaba: sellarla como una hoja valida del arbol.
    index.create_empty_tree();

    // Reservamos e inicializamos la primera pagina de datos (heap de la tabla)
    first_data_page = buffer.allocatePage();
    last_data_page = first_data_page;

    Page* page = buffer.fetchPage(first_data_page);
    SlottedPage::init(*page, first_data_page);
    buffer.unpinPage(first_data_page, true);
    buffer.flushPage(first_data_page);
}

bool Table::insert_record(int key, const std::string& record){
    RID rid;

    Page* page = buffer.fetchPage(last_data_page);
    if(page == nullptr) return false;

    if(!SlottedPage::is_initialized(*page))
        SlottedPage::init(*page, last_data_page);

    bool ok = SlottedPage::insert_record(*page, record.c_str(), record.size(), rid);

    if(!ok){
        // la pagina actual esta llena: creamos una nueva y la enlazamos
        buffer.unpinPage(last_data_page, false);

        int new_page_id = buffer.allocatePage();
        Page* new_page = buffer.fetchPage(new_page_id);
        if(new_page == nullptr) return false;

        SlottedPage::init(*new_page, new_page_id);
        ok = SlottedPage::insert_record(*new_page, record.c_str(), record.size(), rid);

        if(!ok){
            buffer.unpinPage(new_page_id, false);
            return false;
        }

        // enlazar la pagina vieja -> nueva (lista de paginas de datos)
        Page* prev = buffer.fetchPage(last_data_page);
        prev->next_page = new_page_id;
        buffer.unpinPage(last_data_page, true);
        buffer.flushPage(last_data_page);

        last_data_page = new_page_id;
        buffer.unpinPage(new_page_id, true);
        buffer.flushPage(new_page_id);
    } else {
        buffer.unpinPage(last_data_page, true);
        buffer.flushPage(last_data_page);
    }

    return index.insert(key, rid);
}

bool Table::search_record(int key, std::string& out_record){
    RID rid;
    if(!index.search(key, rid)) return false;

    Page* page = buffer.fetchPage(rid.page_id);
    if(page == nullptr) return false;

    char buf[PAGE_SIZE];
    uint16_t size = 0;
    bool ok = SlottedPage::get_record(*page, rid.slot_id, buf, size);

    buffer.unpinPage(rid.page_id, false);

    if(ok) out_record.assign(buf, size);
    return ok;
}