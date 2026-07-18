#ifndef NESTEDLOOPJOIN_H
#define NESTEDLOOPJOIN_H

#include <functional>
#include "Iterator.h"

// Nested Loop Join: por cada fila de 'left', recorre TODO 'right' (que
// se vuelve a abrir DESDE CERO para cada fila izquierda, via
// right_factory, porque el modelo Volcano exige poder reiniciar el lado
// interno) buscando filas que cumplan el predicado de join.
//
// La fila resultante combina ambas: key = clave de la fila izquierda,
// fields = [campos de left] + [clave de right como texto] + [campos de right].
//
// right_factory es un std::function<Iterator*()> en vez de un Iterator*
// fijo, precisamente porque hace falta poder crear una instancia NUEVA
// (o reabrir una) del lado derecho por cada fila izquierda.
class NestedLoopJoin : public Iterator {
private:
    Iterator* left;
    std::function<Iterator*()> right_factory;
    std::function<bool(const Row&, const Row&)> predicate;

    Row current_left;
    bool left_valid;
    Iterator* current_right;

    void open_right_for_current_left();

public:
    NestedLoopJoin(Iterator* left_it,
                   std::function<Iterator*()> right_it_factory,
                   std::function<bool(const Row&, const Row&)> pred);

    void open() override;
    bool next(Row& out) override;
    void close() override;
};

#endif