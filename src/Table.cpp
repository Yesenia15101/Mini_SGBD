#include "../include/Table.h"

Table::ResolveResult Table::resolve_ids(BufferManager& bm, Catalog& catalog, const std::string& name){
    int root, data;

    if(catalog.get_table(name, root, data))
        return { root, data, true };

    int new_root = bm.allocatePage();
    int new_data = bm.allocatePage();

    return { new_root, new_data, false };
}

int Table::find_last_page(int first_page){
    int current = first_page;

    while(true){
        Page* page = buffer.fetchPage(current);
        if(page == nullptr)
            break;

        int next = page->next_page;
        buffer.unpinPage(current, false);

        if(next == -1)
            break;

        current = next;
    }

    return current;
}

// Constructor delegado: aqui ocurre la logica real, ya con los ids resueltos
Table::Table(BufferManager& bm, Catalog& catalog, const std::string& table_name, ResolveResult r)
    : buffer(bm), index(bm, r.root), name(table_name),
      first_data_page(r.data), last_data_page(r.data)
{
    if(r.existed){
        // Tabla reabierta: NO tocar el arbol (ya tiene su magic y sus
        // claves), solo ubicar la pagina de datos realmente "abierta"
        // recorriendo la lista enlazada desde first_data_page.
        last_data_page = find_last_page(first_data_page);
    } else {
        // Tabla nueva: sellar la raiz del arbol como hoja valida,
        // inicializar la primera pagina de datos, y registrar en el Catalog.
        index.create_empty_tree();

        Page* page = buffer.fetchPage(first_data_page);
        SlottedPage::init(*page, first_data_page);
        buffer.unpinPage(first_data_page, true);
        buffer.flushPage(first_data_page);

        catalog.register_table(table_name, r.root, r.data);
    }
}

Table::Table(BufferManager& bm, Catalog& catalog, const std::string& table_name)
    : Table(bm, catalog, table_name, resolve_ids(bm, catalog, table_name))
{}

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

bool Table::insert_row(int key, const std::vector<std::string>& fields){
    // Guardamos la clave TAMBIEN como primera columna del registro fisico.
    // Esto permite que Scan (que recorre paginas de datos SIN tocar el
    // indice) pueda recuperar la clave de cada fila sin depender del B+Tree.
    std::vector<std::string> full_row;
    full_row.reserve(fields.size() + 1);
    full_row.push_back(std::to_string(key));
    full_row.insert(full_row.end(), fields.begin(), fields.end());

    return insert_record(key, Record::serialize(full_row));
}

bool Table::search_row(int key, std::vector<std::string>& out_fields){
    std::string raw;
    if(!search_record(key, raw))
        return false;

    std::vector<std::string> full_row = Record::deserialize(raw);

    if(full_row.empty())
        return false;

    // el primer campo es la clave (el llamador ya la conoce), la quitamos
    out_fields.assign(full_row.begin() + 1, full_row.end());
    return true;
}