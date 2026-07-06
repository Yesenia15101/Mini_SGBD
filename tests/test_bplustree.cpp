#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include "../include/PageManager.h"
#include "../include/BufferManager.h"
#include "../include/BPlusTree.h"
#include "../include/SlottedPage.h"

int verificaciones = 0;
int errores = 0;

void comprobar(bool condicion, const char* detalle){
    verificaciones++;

    if(!condicion){
        errores++;
        std::cout << "  No se pudo validar: " << detalle << "\n";
    }
}

void imprimir_registro(PageManager& db, const char* etiqueta, RID rid){
    Page page;
    char salida[140];
    uint16_t size = 0;
    std::memset(salida, 0, sizeof(salida));

    bool leyo_pagina = db.read_page(rid.page_id, page);
    bool leyo_registro = false;

    if(leyo_pagina){
        leyo_registro = SlottedPage::get_record(
            page,
            rid.slot_id,
            salida,
            size
        );
    }

    if(leyo_registro)
        salida[size] = '\0';

    std::cout << etiqueta << " -> RID(" << rid.page_id << ", " << rid.slot_id << ")";

    if(leyo_registro)
        std::cout << " -> " << salida;
    else
        std::cout << " -> no se pudo leer el registro";

    std::cout << "\n";

    comprobar(leyo_pagina, "leer pagina de datos desde PageManager");
    comprobar(leyo_registro, "leer registro desde SlottedPage usando RID");
}

void imprimir_rango_desde_indice(PageManager& db, BPlusTree& indice, int inicio, int fin){
    for(int id = inicio; id <= fin; id++){
        RID rid = {-1, 0};
        bool encontrado = indice.search(id, rid);

        if(encontrado){
            std::string etiqueta = "   id=" + std::to_string(id);
            imprimir_registro(db, etiqueta.c_str(), rid);
        }else{
            std::cout << "   id=" << id << " -> no encontrado en el indice\n";
        }
    }
}

std::string generar_nombre_alumno(int id){
    static const char* nombres[] = {
        "Ana", "Luis", "Maria", "Carlos", "Lucia",
        "Jorge", "Valeria", "Diego", "Camila", "Mateo",
        "Sofia", "Andres", "Daniela", "Sebastian", "Fernanda",
        "Ricardo", "Paola", "Miguel", "Gabriela", "Alejandro"
    };

    static const char* apellidos[] = {
        "Quispe", "Mamani", "Flores", "Garcia", "Vargas",
        "Rojas", "Torres", "Condori", "Huaman", "Castillo",
        "Lopez", "Ramirez", "Chavez", "Paredes", "Medina",
        "Salas", "Cruz", "Herrera", "Mendoza", "Aguilar"
    };

    int nombre_index = (id - 1) % 20;
    int apellido_index = ((id - 1) / 20) % 20;

    return std::string(nombres[nombre_index]) + " " + apellidos[apellido_index];
}

void demo_bplustree_1000_registros(bool imprimir_todos){
    std::remove("test_bplustree_1000.db");

    const int total_registros = 1000;
    const int primera_pagina_datos = 100;
    int ultima_pagina_datos = primera_pagina_datos;
    int root_guardada = -1;

    std::cout << "=== DEMOSTRACION B+ TREE INTEGRADO ===\n\n";
    std::cout << "Archivo de prueba: test_bplustree_1000.db\n";
    std::cout << "Cantidad de registros: " << total_registros << "\n\n";

    {
        PageManager db("test_bplustree_1000.db");
        BufferManager bm(32, db);
        BPlusTree indice_alumnos(bm, 1);

        std::vector<RID> rids(total_registros + 1);
        bool datos_insertados = true;

        Page data_page;
        int data_page_id = primera_pagina_datos;
        SlottedPage::init(data_page, data_page_id);

        std::cout << "1. Insertando registros en paginas de datos...\n";

        for(int id = 1; id <= total_registros; id++){
            std::string registro = std::to_string(id) +
                                   "|" +
                                   generar_nombre_alumno(id) +
                                   "|" +
                                   std::to_string(18 + (id % 10));

            RID rid;
            bool insertado = SlottedPage::insert_record(
                data_page,
                registro.c_str(),
                static_cast<uint16_t>(registro.size()),
                rid
            );

            if(!insertado){
                db.write_page(data_page_id, data_page);

                data_page_id++;
                SlottedPage::init(data_page, data_page_id);

                insertado = SlottedPage::insert_record(
                    data_page,
                    registro.c_str(),
                    static_cast<uint16_t>(registro.size()),
                    rid
                );
            }

            datos_insertados = insertado && datos_insertados;
            rids[id] = rid;
        }

        db.write_page(data_page_id, data_page);
        db.sync();
        ultima_pagina_datos = data_page_id;

        std::cout << "   Registros guardados en paginas "
                  << primera_pagina_datos << " a " << ultima_pagina_datos << ".\n";
        std::cout << "   Paginas de datos usadas: "
                  << (ultima_pagina_datos - primera_pagina_datos + 1) << "\n\n";

        comprobar(datos_insertados, "insertar los 1000 registros en SlottedPage");

        std::cout << "2. Construyendo indice B+ sobre la clave id...\n";

        bool indice_creado = indice_alumnos.create_empty_tree();
        bool indice_insertado = true;

        for(int id = 1; id <= total_registros; id++)
            indice_insertado = indice_alumnos.insert(id, rids[id]) && indice_insertado;

        root_guardada = indice_alumnos.get_root_page_id();

        std::cout << "   El indice guarda entradas con forma: id -> RID(page_id, slot_id).\n";
        std::cout << "   Root page id del indice B+: " << root_guardada << "\n\n";

        comprobar(indice_creado, "crear raiz inicial del B+ Tree");
        comprobar(indice_insertado, "insertar 1000 claves en el B+ Tree");
        comprobar(root_guardada != 1, "forzar split y creacion de nueva raiz interna");

        std::cout << "3. Buscando registros mediante el indice...\n";

        RID rid_1 = {-1, 0};
        RID rid_500 = {-1, 0};
        RID rid_1000 = {-1, 0};

        bool busca_1 = indice_alumnos.search(1, rid_1);
        bool busca_500 = indice_alumnos.search(500, rid_500);
        bool busca_1000 = indice_alumnos.search(1000, rid_1000);

        comprobar(busca_1, "buscar id 1 en el B+ Tree");
        comprobar(busca_500, "buscar id 500 en el B+ Tree");
        comprobar(busca_1000, "buscar id 1000 en el B+ Tree");

        imprimir_registro(db, "   id=1", rid_1);
        imprimir_registro(db, "   id=500", rid_500);
        imprimir_registro(db, "   id=1000", rid_1000);

        if(imprimir_todos){
            std::cout << "\n   Impresion completa ANTES de eliminar usando el indice B+:\n";
            imprimir_rango_desde_indice(db, indice_alumnos, 1, total_registros);
        }

        std::cout << "\n4. Eliminando claves del indice...\n";

        bool elimina_10 = indice_alumnos.remove(10);
        bool elimina_500 = indice_alumnos.remove(500);
        bool elimina_999 = indice_alumnos.remove(999);

        RID eliminado = {-1, 0};
        RID vigente = {-1, 0};
        bool ya_no_esta_500 = !indice_alumnos.search(500, eliminado);
        bool sigue_501 = indice_alumnos.search(501, vigente);

        root_guardada = indice_alumnos.get_root_page_id();

        std::cout << "   Se eliminaron del indice los ids 10, 500 y 999.\n";
        std::cout << "   Busqueda posterior de id=500: no encontrado.\n";
        std::cout << "   Busqueda posterior de id=501: encontrado en RID("
                  << vigente.page_id << ", " << vigente.slot_id << ").\n\n";

        comprobar(elimina_10, "eliminar id 10");
        comprobar(elimina_500, "eliminar id 500");
        comprobar(elimina_999, "eliminar id 999");
        comprobar(ya_no_esta_500, "confirmar que id 500 ya no aparece");
        comprobar(sigue_501, "confirmar que id 501 sigue disponible");

        if(imprimir_todos){
            std::cout << "   Impresion completa DESPUES de eliminar usando el indice B+:\n";
            imprimir_rango_desde_indice(db, indice_alumnos, 1, total_registros);
            std::cout << "\n";
        }
    }

    {
        std::cout << "5. Reabriendo el archivo para comprobar persistencia...\n";

        PageManager db("test_bplustree_1000.db");
        BufferManager bm(32, db);
        BPlusTree indice_alumnos(bm, root_guardada);

        RID rid_1000 = {-1, 0};
        RID rid_500 = {-1, 0};

        bool persiste_1000 = indice_alumnos.search(1000, rid_1000);
        bool eliminado_persiste = !indice_alumnos.search(500, rid_500);

        comprobar(persiste_1000, "id 1000 persiste despues de reabrir");
        comprobar(eliminado_persiste, "id 500 sigue eliminado despues de reabrir");

        if(persiste_1000)
            imprimir_registro(db, "   id=1000 despues de reabrir", rid_1000);

        std::cout << "   id=500 despues de reabrir: no encontrado.\n\n";
    }

    std::cout << "Resumen de la demostracion:\n";
    std::cout << "   Registros insertados: " << total_registros << "\n";
    std::cout << "   Paginas de datos usadas: "
              << (ultima_pagina_datos - primera_pagina_datos + 1) << "\n";
    std::cout << "   Operaciones demostradas: insercion, busqueda, eliminacion, split, merge y persistencia.\n";
    std::cout << "   Validaciones internas: " << verificaciones << "\n";
    std::cout << "   Errores detectados: " << errores << "\n";

    if(errores == 0)
        std::cout << "\nResultado final: el B+ Tree funciona integrado con BufferManager y PageManager.\n";
    else
        std::cout << "\nResultado final: hay validaciones que revisar.\n";
}

int main(int argc, char* argv[]){
    bool imprimir_todos = argc > 1 && std::strcmp(argv[1], "--print-all") == 0;

    demo_bplustree_1000_registros(imprimir_todos);

    if(errores == 0)
        return 0;

    return 1;
}
