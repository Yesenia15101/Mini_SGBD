#ifndef CATALOG_H
#define CATALOG_H
 
#include <string>
#include <cstdint>
#include "BufferManager.h"
#include "Page.h"
 
// Catalog resuelve el problema de persistencia: sin el, cada vez que el
// programa arranca, Table pide paginas NUEVAS via buffer.allocatePage(),
// y los datos de la ejecucion anterior quedan huerfanos en el archivo
// (existen en disco, pero nadie recuerda en que pagina estaba su raiz
// de indice ni su primera pagina de datos).
//
// Catalog reserva SIEMPRE la pagina 0 del archivo para guardar esa
// informacion (root_page_id y first_data_page por cada tabla, indexados
// por nombre). Al reabrir la base de datos, Table pregunta al Catalog
// si ya existe una tabla con ese nombre; si existe, reutiliza esos
// mismos page_id en vez de crear unos nuevos.
//
// IMPORTANTE: Catalog debe construirse ANTES que cualquier Table, y
// antes de cualquier llamada a buffer.allocatePage() en el programa.
// Motivo: si el archivo esta vacio, la PRIMERA pagina que se allocate
// sera la 0 (PageManager calcula el id como tamaño_actual/PAGE_SIZE).
// Al construirse, Catalog escribe en la pagina 0 inmediatamente, lo que
// hace crecer el archivo a 1 pagina, y asi la siguiente allocatePage()
// (la que pida Table para su raiz) obtiene correctamente la pagina 1,
// sin chocar con la del catalogo.
class Catalog {
private:
    static const uint16_t MAGIC = 0xCA7A;
    static const int CATALOG_PAGE_ID = 0;
    static const int MAX_TABLES = 8;
 
#pragma pack(push, 1)
    struct TableEntry {
        char name[32];
        int root_page_id;
        int first_data_page;
        uint8_t in_use;
    };
 
    struct CatalogHeader {
        uint16_t magic;
        uint16_t table_count;
    };
#pragma pack(pop)
 
    BufferManager& buffer;
 
    static CatalogHeader* header(Page& page);
    static TableEntry* entry(Page& page, int index);
 
public:
    explicit Catalog(BufferManager& bm);
 
    // true si la tabla ya existia (y llena root_page_id/first_data_page
    // con los valores guardados); false si no hay registro con ese nombre.
    bool get_table(const std::string& name, int& root_page_id, int& first_data_page);
 
    // Registra una tabla nueva. Falla si el nombre ya existe o si se
    // llego al limite MAX_TABLES (limite fijo, suficiente para el proyecto).
    bool register_table(const std::string& name, int root_page_id, int first_data_page);
};
 
#endif