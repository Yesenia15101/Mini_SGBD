#include <iostream>
#include <cstdio>
#include <cstring>
#include "../include/PageManager.h"
#include "../include/SlottedPage.h"
#include "../include/BufferManager.h"

void mostrar_registro(Page& page, uint16_t slot_id){
    char salida[200];
    uint16_t size = 0;

    std::memset(salida, 0, sizeof(salida));

    if(SlottedPage::get_record(page, slot_id, salida, size)){
        salida[size] = '\0';
        std::cout << "Slot " << slot_id << ": " << salida << "\n";
    }else{
        std::cout << "Slot " << slot_id << ": no disponible\n";
    }
}

int main(){
    PageManager db("mibasedatos.db");

    if(!db.is_open()){
        std::cout << "No se pudo abrir la base de datos.\n";
        return 1;
    }

    Page page;
    SlottedPage::init(page, 2);

    RID r1, r2, r3;

    const char* reg1 = "1|Ana|20";
    const char* reg2 = "2|Luis Alberto|21";
    const char* reg3 = "3|Maria Fernanda Mamani|22";

    SlottedPage::insert_record(page, reg1, std::strlen(reg1), r1);
    SlottedPage::insert_record(page, reg2, std::strlen(reg2), r2);
    SlottedPage::insert_record(page, reg3, std::strlen(reg3), r3);

    std::cout << "Registros insertados en pagina " << page.page_id << "\n";
    std::cout << "RID 1 = (" << r1.page_id << ", " << r1.slot_id << ")\n";
    std::cout << "RID 2 = (" << r2.page_id << ", " << r2.slot_id << ")\n";
    std::cout << "RID 3 = (" << r3.page_id << ", " << r3.slot_id << ")\n";

    std::cout << "\nContenido antes de guardar en disco:\n";
    mostrar_registro(page, 0);
    mostrar_registro(page, 1);
    mostrar_registro(page, 2);

    std::cout << "\nSlots usados: " << SlottedPage::get_slot_count(page) << "\n";
    std::cout << "Espacio libre: " << SlottedPage::get_free_space(page) << " bytes\n";

    if(!SlottedPage::validate(page)){
        std::cout << "La pagina no es valida.\n";
        return 1;
    }

    if(!db.write_page(2, page)){
        std::cout << "Error al escribir la pagina.\n";
        return 1;
    }

    if(!db.sync()){
        std::cout << "Error al sincronizar con disco.\n";
        return 1;
    }

    std::cout << "\nPagina guardada y sincronizada con fsync/FlushFileBuffers.\n";

    Page recuperada;

    if(!db.read_page(2, recuperada)){
        std::cout << "Error al leer la pagina desde disco.\n";
        return 1;
    }

    std::cout << "\nContenido recuperado desde disco:\n";
    mostrar_registro(recuperada, 0);
    mostrar_registro(recuperada, 1);
    mostrar_registro(recuperada, 2);

    std::cout << "\nEliminando logicamente el slot 1...\n";

    SlottedPage::delete_record(recuperada, 1);

    mostrar_registro(recuperada, 0);
    mostrar_registro(recuperada, 1);
    mostrar_registro(recuperada, 2);

    RID r4;
    const char* reg4 = "4|Carlos|23";

    SlottedPage::insert_record(recuperada, reg4, std::strlen(reg4), r4);

    std::cout << "\nInsertando nuevo registro reutilizando slot eliminado...\n";
    std::cout << "RID 4 = (" << r4.page_id << ", " << r4.slot_id << ")\n";

    mostrar_registro(recuperada, 0);
    mostrar_registro(recuperada, 1);
    mostrar_registro(recuperada, 2);

    db.write_page(2, recuperada);
    db.sync();

    std::cout << "\nPrueba de Slot Directory finalizada correctamente.\n";

    std::cout << "\n===== BUFFER MANAGER TEST =====\n";

    BufferManager bm(2, db);

    Page* p1 = bm.fetchPage(2);

    if(p1){
        std::cout << "Pagina 2 cargada en RAM correctamente.\n";
    }else{
        std::cout << "Error cargando pagina 2.\n";
    }
    Page page_extra;

    SlottedPage::init(page_extra, 3);

    db.write_page(3, page_extra);

    Page* p3 = bm.fetchPage(3);

    if(p3){
        std::cout << "Pagina 3 cargada.\n";
    }
    Page another;

    SlottedPage::init(another, 4);

    db.write_page(4, another);

    Page* p4 = bm.fetchPage(4);

    if(p4 == nullptr){
        std::cout << "Buffer Pool lleno.\n";
    }

    return 0;
}