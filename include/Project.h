#ifndef PROJECT_H
#define PROJECT_H

#include <vector>
#include "Iterator.h"

// Project: se queda solo con las columnas indicadas en 'column_indices'
// (indices dentro de row.fields; la clave 'key' siempre se conserva,
// ya que suele ser necesaria para operadores posteriores como Join).
class Project : public Iterator {
private:
    Iterator* child;
    std::vector<int> column_indices;

public:
    Project(Iterator* child_it, std::vector<int> indices)
        : child(child_it), column_indices(std::move(indices)) {}

    void open() override { child->open(); }

    bool next(Row& out) override {
        Row full;
        if(!child->next(full))
            return false;

        out.key = full.key;
        out.fields.clear();

        for(int idx : column_indices){
            if(idx >= 0 && idx < static_cast<int>(full.fields.size()))
                out.fields.push_back(full.fields[idx]);
            else
                out.fields.push_back(""); // columna fuera de rango
        }

        return true;
    }

    void close() override { child->close(); }
};

#endif