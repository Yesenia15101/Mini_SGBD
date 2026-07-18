#include "../include/Scan.h"
#include "../include/SlottedPage.h"
#include "../include/Record.h"

Scan::Scan(BufferManager& bm, int first_data_page_id)
    : buffer(bm), first_data_page(first_data_page_id),
      current_page_id(-1), current_slot_id(0), current_page(nullptr) {}

void Scan::open(){
    current_page_id = first_data_page;
    current_slot_id = 0;
    current_page = buffer.fetchPage(current_page_id);
}

bool Scan::next(Row& out){
    while(current_page != nullptr){
        uint16_t slot_count = SlottedPage::get_slot_count(*current_page);

        while(current_slot_id < slot_count){
            char buf[PAGE_SIZE];
            uint16_t size = 0;
            uint16_t slot = current_slot_id;
            current_slot_id++;

            if(SlottedPage::get_record(*current_page, slot, buf, size)){
                std::string raw(buf, size);
                std::vector<std::string> row_fields = Record::deserialize(raw);

                if(row_fields.empty())
                    continue; // fila vacia/corrupta: saltar

                out.key = std::stoi(row_fields[0]); // Table guarda la clave como 1er campo
                out.fields.assign(row_fields.begin() + 1, row_fields.end());
                return true;
            }
            // slot borrado o libre: seguir con el siguiente slot
        }

        // se acabaron los slots de esta pagina: pasar a la siguiente del enlace
        int next_page_id = current_page->next_page;
        buffer.unpinPage(current_page_id, false);

        current_page_id = next_page_id;
        current_slot_id = 0;

        current_page = (current_page_id == -1) ? nullptr : buffer.fetchPage(current_page_id);
    }

    return false;
}

void Scan::close(){
    if(current_page != nullptr){
        buffer.unpinPage(current_page_id, false);
        current_page = nullptr;
    }
}