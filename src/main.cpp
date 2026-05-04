
#include <iostream>
#include "../include/PageManager.h"

int main() {
    PageManager manager("mibasedatos.db");
    Page p1;
    p1.page_id = 7;
    p1.data_count = 100;
    snprintf(p1.buffer, 50, "Buenas, esto es binario puro!");

    manager.write_page(10, p1);
    std::cout << "Pagina 10 guardada correctamente." << std::endl;

    Page p2 = manager.read_page(10);
    
    std::cout << "Pagina recuperada. ID: " << p2.page_id << std::endl;
    std::cout << "Contenido del buffer: " << p2.buffer << std::endl;

    return 0;
}