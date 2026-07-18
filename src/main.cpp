
#include <iostream>
#include <string>
#include <vector>

#include "../include/PageManager.h"
#include "../include/BufferManager.h"
#include "../include/Catalog.h"
#include "../include/Table.h"

#include "../include/Iterator.h"
#include "../include/Scan.h"
#include "../include/Select.h"
#include "../include/Project.h"
#include "../include/IndexScan.h"
#include "../include/NestedLoopJoin.h"

void print_row(const Row& r){
    std::cout << "  key=" << r.key << "  [ ";
    for(const auto& f : r.fields) std::cout << f << " | ";
    std::cout << "]\n";
}

int main()
{
    std::remove("database.bin");

    PageManager disk("database.bin");
    if(!disk.is_open()){
        std::cout << "No se pudo abrir la base de datos\n";
        return 0;
    }

    BufferManager buffer(10, disk);
    Catalog catalog(buffer);
    Table alumnos(buffer, catalog, "Alumno");

    // columnas: nombre, edad, pais
    alumnos.insert_row(10, {"Juan",   "18", "Peru"});
    alumnos.insert_row(25, {"Maria",  "22", "Colombia"});
    alumnos.insert_row(15, {"Pedro",  "20", "Peru"});
    alumnos.insert_row(7,  {"Carlos", "19", "Chile"});
    alumnos.insert_row(40, {"Jose",   "21", "Peru"});
    alumnos.insert_row(50, {"Ana",    "23", "Chile"});

    int first_page = alumnos.get_first_data_page_id();

    //=====================================================
    // 1. SCAN puro: todas las filas, sin filtrar ni proyectar
    //=====================================================
    std::cout << "\n===== 1. SCAN completo =====\n";
    {
        Scan scan(buffer, first_page);
        scan.open();
        Row row;
        while(scan.next(row)) print_row(row);
        scan.close();
    }

    //=====================================================
    // 2. SELECT + PROJECT: edad > 19, proyectando solo nombre y pais
    //    (fields[0]=nombre, fields[1]=edad, fields[2]=pais)
    //=====================================================
    std::cout << "\n===== 2. SELECT edad>19 + PROJECT(nombre,pais) =====\n";
    {
        Scan* base = new Scan(buffer, first_page);

        Select select(base, [](const Row& r){
            return std::stoi(r.fields[1]) > 19;
        });

        Project project(&select, {0, 2}); // nombre, pais

        project.open();
        Row row;
        while(project.next(row)) print_row(row);
        project.close();

        delete base;
    }

    //=====================================================
    // 3. NESTED LOOP JOIN: pares de alumnos DEL MISMO PAIS
    //    (self-join de Alumno consigo misma)
    //=====================================================
    std::cout << "\n===== 3. NESTED LOOP JOIN (mismo pais, self-join) =====\n";
    {
        Scan* left = new Scan(buffer, first_page);

        auto right_factory = [&buffer, first_page]() -> Iterator* {
            return new Scan(buffer, first_page);
        };

        auto same_country_diff_key = [](const Row& l, const Row& r){
            return l.key != r.key && l.fields[2] == r.fields[2];
        };

        NestedLoopJoin join(left, right_factory, same_country_diff_key);

        join.open();
        Row row;
        while(join.next(row)) print_row(row);
        join.close();

        delete left;
    }

    //=====================================================
    // 4. INDICE vs SCAN: comparar cuantos fetches hacen falta
    //    para encontrar UNA clave especifica (key = 40)
    //=====================================================
    std::cout << "\n===== 4. Optimizacion con indice: IndexScan vs Scan+Select =====\n";

    // --- 4a. Sin indice: Scan + Select recorriendo TODO ---
    buffer.reset_stats();
    {
        Scan* base = new Scan(buffer, first_page);
        Select find_by_key(base, [](const Row& r){ return r.key == 40; });

        find_by_key.open();
        Row row;
        bool found = find_by_key.next(row);
        find_by_key.close();

        std::cout << "Scan+Select (sin indice): encontrado=" << found
                   << "  fetches totales=" << (buffer.get_hit_count() + buffer.get_miss_count())
                   << " (hits=" << buffer.get_hit_count()
                   << ", misses=" << buffer.get_miss_count() << ")\n";

        delete base;
    }

    // --- 4b. Con indice: IndexScan (usa el B+Tree por debajo) ---
    buffer.reset_stats();
    {
        IndexScan find_by_key(alumnos, 40);

        find_by_key.open();
        Row row;
        bool found = find_by_key.next(row);
        find_by_key.close();

        std::cout << "IndexScan   (con indice): encontrado=" << found
                   << "  fetches totales=" << (buffer.get_hit_count() + buffer.get_miss_count())
                   << " (hits=" << buffer.get_hit_count()
                   << ", misses=" << buffer.get_miss_count() << ")\n";
    }

    std::cout << "\nHit rate acumulado de toda la corrida: "
               << (buffer.get_hit_rate() * 100.0) << "%\n";

    return 0;
}




/*
#include <iostream>
#include <string>
#include <vector>

#include "../include/PageManager.h"
#include "../include/BufferManager.h"
#include "../include/Catalog.h"
#include "../include/Table.h"

int main(){
    // -----------------------------------------------------------------
    // PRUEBA DE PERSISTENCIA:
    //
    // 1ra corrida: deja esta linea de std::remove ACTIVA. Correlo, veras
    //              que inserta filas y las encuentra.
    // 2da corrida: COMENTA la linea de std::remove (o borrala) y corre
    //              de nuevo. Deberias ver "Tabla YA EXISTIA" y las
    //              busquedas de las claves de ayer (10, 25, 15, 7, 40)
    //              deberian seguir funcionando SIN volver a insertarlas.
    // -----------------------------------------------------------------
    //std::remove("database.bin"); // <- comenta esta linea para probar persistencia real

    PageManager disk("database.bin");

    if(!disk.is_open()){
        std::cout << "No se pudo abrir la base de datos\n";
        return 0;
    }

    BufferManager buffer(10, disk);

    // Catalog SIEMPRE se construye antes que cualquier Table
    Catalog catalog(buffer);

    int check_root, check_data;
    bool already_existed = catalog.get_table("Alumno", check_root, check_data);

    std::cout << (already_existed ? "Tabla 'Alumno' YA EXISTIA (persistencia funcionando)\n"
                                   : "Tabla 'Alumno' es NUEVA\n");

    Table alumnos(buffer, catalog, "Alumno");

    std::cout << "root_page_id(indice) = " << alumnos.get_index_root_page_id()
              << ", primera pagina de datos = " << alumnos.get_first_data_page_id() << "\n\n";

    if(!already_existed){
        // Filas con varios campos: (id, nombre, edad, pais)
        // insert_row serializa los campos con Record::serialize antes de guardarlos
        alumnos.insert_row(10, {"Juan", "18", "Peru"});
        alumnos.insert_row(25, {"Maria", "22", "Colombia"});
        alumnos.insert_row(15, {"Pedro", "20", "Peru"});
        alumnos.insert_row(7,  {"Carlos", "19", "Chile"});
        alumnos.insert_row(40, {"Jose", "21", "Peru"});

        std::cout << "5 filas insertadas.\n\n";
    }

    // Busquedas (funcionan igual, haya insertado en esta corrida o en una anterior)
    int claves_a_buscar[] = {10, 25, 15, 7, 40, 999};

    for(int k : claves_a_buscar){
        std::vector<std::string> fields;

        if(alumnos.search_row(k, fields)){
            std::cout << "key=" << k << "  ->  nombre=" << fields[0]
                       << ", edad=" << fields[1]
                       << ", pais=" << fields[2] << "\n";
        } else {
            std::cout << "key=" << k << "  ->  NO ENCONTRADO\n";
        }
    }

    return 0;
}
*/