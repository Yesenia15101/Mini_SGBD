#ifndef TABLE_H
#define TABLE_H

#include <string>
#include <vector>
#include "BufferManager.h"
#include "BPlusTree.h"
#include "SlottedPage.h"
#include "Catalog.h"
#include "Record.h"

// Table encapsula todo lo necesario para insertar y buscar registros:
// - Si la tabla YA EXISTIA (segun el Catalog), reutiliza su root_page_id
//   y su first_data_page, y localiza la ultima pagina de datos real
//   recorriendo la lista enlazada (next_page) desde first_data_page.
// - Si NO existia, reserva paginas nuevas, inicializa el arbol y la
//   primera pagina de datos, y se registra en el Catalog.
class Table {
private:
    struct ResolveResult {
        int root;
        int data;
        bool existed;
    };

    BufferManager& buffer;
    BPlusTree index;
    std::string name;
    int first_data_page;
    int last_data_page;

    static ResolveResult resolve_ids(BufferManager& bm, Catalog& catalog, const std::string& name);
    int find_last_page(int first_page);

    // Constructor delegado: recibe ya resueltos los ids (existentes o nuevos)
    Table(BufferManager& bm, Catalog& catalog, const std::string& name, ResolveResult r);

public:
    Table(BufferManager& bm, Catalog& catalog, const std::string& table_name);

    // API de bajo nivel: un registro es un string crudo (como antes)
    bool insert_record(int key, const std::string& record);
    bool search_record(int key, std::string& out_record);

    // API de mas alto nivel: un registro es una fila con varios campos,
    // serializados/deserializados via Record (ej. {"20","Juan","18","Peru"})
    bool insert_row(int key, const std::vector<std::string>& fields);
    bool search_row(int key, std::vector<std::string>& out_fields);

    int get_index_root_page_id() const { return index.get_root_page_id(); }
    int get_first_data_page_id() const { return first_data_page; }
};

#endif