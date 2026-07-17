#ifndef TABLE_H
#define TABLE_H
 
#include <string>
#include "BufferManager.h"
#include "BPlusTree.h"
#include "SlottedPage.h"
 
// Table encapsula TODO lo necesario para insertar y buscar registros:
// - Reserva e inicializa su propia pagina raiz de indice (B+Tree).
// - Reserva e inicializa su propia primera pagina de datos (SlottedPage).
// - insert_record() hace: guardar el dato crudo -> obtener RID -> indexar el RID.
//
// NOTA IMPORTANTE: este constructor siempre crea una tabla NUEVA (alloca
// paginas frescas). Si mas adelante necesitas reabrir una tabla que ya
// existia en el archivo .bin (persistencia real entre ejecuciones), vamos
// a necesitar una segunda forma de construccion que reciba los page_id ya
// existentes en vez de allocar nuevos -- eso es trabajo para un futuro
// "Catalog" que recuerde esos ids entre ejecuciones. Por ahora, para
// pruebas y para el modelo Volcano, esto es suficiente.
class Table {
private:
    BufferManager& buffer;
    BPlusTree index;
    int first_data_page;
    int last_data_page;
 
public:
    explicit Table(BufferManager& bm);
 
    bool insert_record(int key, const std::string& record);
    bool search_record(int key, std::string& out_record);
 
    int get_index_root_page_id() const { return index.get_root_page_id(); }
    int get_first_data_page_id() const { return first_data_page; }
};
 
#endif