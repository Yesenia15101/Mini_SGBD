#ifndef ITERATOR_H
#define ITERATOR_H

#include <string>
#include <vector>

// Row: una fila del modelo de consultas.
// key    = el identificador (la columna indexada por el B+Tree).
// fields = las demas columnas, como texto.
struct Row {
    int key;
    std::vector<std::string> fields;
};

// Interfaz del Modelo de Iterador (Volcano Model). Todo operador de
// consulta (Scan, Select, Project, Join, IndexScan...) implementa este
// contrato de 3 metodos:
//   open()  -> prepara el operador para empezar a producir filas
//   next()  -> produce UNA fila mas; retorna false cuando ya no hay mas
//   close() -> libera recursos (cierra iteradores hijos, despinea paginas)
//
// La clave del modelo Volcano es que cada operador solo conoce la
// INTERFAZ de su(s) hijo(s), nunca su implementacion. Asi, Select no
// sabe si su hijo es un Scan o un Join -- simplemente le pide next().
class Iterator {
public:
    virtual void open() = 0;
    virtual bool next(Row& out) = 0;
    virtual void close() = 0;
    virtual ~Iterator() {}
};

#endif