#ifndef SCAN_H
#define SCAN_H
 
#include "Iterator.h"
#include "BufferManager.h"
#include "Page.h"
 
// Scan (Sequential Scan): recorre TODAS las filas de una tabla, pagina
// por pagina, siguiendo la lista enlazada (next_page) que arma Table al
// insertar. Es el operador mas basico del modelo Volcano: no filtra ni
// proyecta nada, solo entrega, una por una, todas las filas que existen.
//
// Costo: O(numero de paginas de datos), sin importar cuantas filas
// busques -- por eso mas abajo existe IndexScan como alternativa cuando
// buscas UNA clave especifica.
class Scan : public Iterator {
private:
    BufferManager& buffer;
    int first_data_page;
 
    int current_page_id;
    uint16_t current_slot_id;
    Page* current_page; // pineada mientras se recorre esa pagina; se
                         // suelta apenas se pasa a la siguiente
 
public:
    Scan(BufferManager& bm, int first_data_page_id);
 
    void open() override;
    bool next(Row& out) override;
    void close() override;
};
 
#endif