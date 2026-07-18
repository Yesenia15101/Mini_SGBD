#ifndef SELECT_H
#define SELECT_H

#include <functional>
#include "Iterator.h"

// Select: filtra las filas que le entrega su Iterator hijo, segun un
// predicado arbitrario. Es un operador "pass-through": por cada next(),
// sigue pidiendo filas al hijo hasta encontrar una que cumpla el
// predicado, o hasta que el hijo se agote.
//
// Header-only por simplicidad: es logica pequeña y generica (usa
// std::function), no necesita .cpp separado.
class Select : public Iterator {
private:
    Iterator* child;
    std::function<bool(const Row&)> predicate;

public:
    Select(Iterator* child_it, std::function<bool(const Row&)> pred)
        : child(child_it), predicate(pred) {}

    void open() override { child->open(); }

    bool next(Row& out) override {
        Row candidate;
        while(child->next(candidate)){
            if(predicate(candidate)){
                out = candidate;
                return true;
            }
        }
        return false;
    }

    void close() override { child->close(); }
};

#endif