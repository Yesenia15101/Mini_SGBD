#ifndef INDEXSCAN_H
#define INDEXSCAN_H

#include "Iterator.h"
#include "Table.h"

// IndexScan: en vez de recorrer TODA la tabla (como Scan) para encontrar
// una clave especifica, usa directamente el B+Tree via Table::search_row,
// que es O(log n) en vez de O(n paginas). Este es el operador que
// demuestra "uso del indice B+Tree para optimizar consultas"
// (rubrica, item 15).
//
// Uso tipico: en vez de armar Select(Scan(tabla), pred: key==X), que
// recorre TODA la tabla comparando clave por clave, usas directamente
// IndexScan(tabla, X) para una busqueda puntual por clave.
class IndexScan : public Iterator {
private:
    Table& table;
    int target_key;
    bool done;

public:
    IndexScan(Table& t, int key) : table(t), target_key(key), done(false) {}

    void open() override { done = false; }

    bool next(Row& out) override {
        if(done) return false;
        done = true;

        std::vector<std::string> fields;
        if(!table.search_row(target_key, fields))
            return false;

        out.key = target_key;
        out.fields = fields;
        return true;
    }

    void close() override {}
};

#endif