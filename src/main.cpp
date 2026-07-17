#include <iostream>
#include <cstdio>
#include <string>

#include "../include/PageManager.h"
#include "../include/BufferManager.h"
#include "../include/Table.h"

int main()
{
    // Empezamos limpio para que la demo sea reproducible
    // (si quieres persistencia real entre ejecuciones, quita esta linea,
    // pero recuerda que por ahora Table SIEMPRE crea paginas nuevas al
    // construirse, asi que reabrir un archivo viejo simplemente le
    // agregaria paginas nuevas sin reusar el indice anterior)
    std::remove("database.bin");

    PageManager disk("database.bin");

    if(!disk.is_open())
    {
        std::cout << "No se pudo abrir la base de datos\n";
        return 0;
    }

    BufferManager buffer(10, disk); // 10 frames

    // Table se encarga de TODO: crea su propia pagina raiz de indice,
    // crea su propia primera pagina de datos, e inicializa ambas.
    Table alumnos(buffer);

    std::cout << "Tabla creada. root_page_id(indice) = "
              << alumnos.get_index_root_page_id()
              << ", primera pagina de datos = "
              << alumnos.get_first_data_page_id() << "\n\n";

    //--------------------------------------------------
    // INSERTS
    //--------------------------------------------------

    struct { int key; std::string nombre; } filas[] = {
        {10, "Juan"},
        {25, "Maria"},
        {15, "Pedro"},
        {7,  "Carlos"},
        {40, "Jose"}
    };

    for(auto& f : filas){
        bool ok = alumnos.insert_record(f.key, f.nombre);
        std::cout << (ok ? "[OK]   " : "[FAIL] ")
                  << "insert_record(" << f.key << ", \"" << f.nombre << "\")\n";
    }

    std::cout << "\n";

    //--------------------------------------------------
    // SEARCH (incluyendo una clave que no existe)
    //--------------------------------------------------

    int claves_a_buscar[] = {15, 10, 25, 7, 40, 999};

    for(int k : claves_a_buscar){
        std::string registro;
        if(alumnos.search_record(k, registro))
            std::cout << "key=" << k << "  ->  " << registro << "\n";
        else
            std::cout << "key=" << k << "  ->  NO ENCONTRADO\n";
    }

    return 0;
}