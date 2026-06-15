#include <cstdio>
#include <cstring>
#include <iostream>
#include "../include/PageManager.h"
#include "../include/BufferManager.h"
#include "../include/BPlusTree.h"
#include "../include/SlottedPage.h"

int ok = 0;
int fail = 0;

void verificar(bool condicion, const char* nombre){
    if(condicion){
        std::cout << "[OK] " << nombre << "\n";
        ok++;
    }else{
        std::cout << "[FAIL] " << nombre << "\n";
        fail++;
    }
}

void test_busqueda_en_raiz_hoja(){
    std::remove("test_bplustree.db");

    PageManager db("test_bplustree.db");
    BufferManager bm(4, db);
    BPlusTree tree(bm, 1);

    bool creado = tree.create_empty_tree();

    Page* root = bm.fetchPage(1);
    RID rid10 = {10, 0};
    RID rid20 = {20, 1};
    RID rid30 = {30, 2};

    bool i1 = BPlusTreeNode::set_leaf_entry(*root, 0, 10, rid10);
    bool i2 = BPlusTreeNode::set_leaf_entry(*root, 1, 20, rid20);
    bool i3 = BPlusTreeNode::set_leaf_entry(*root, 2, 30, rid30);

    bm.unpinPage(1, true);
    bm.flushPage(1);

    RID encontrado = {-1, 0};
    bool existe = tree.search(20, encontrado);
    bool no_existe = tree.search(25, encontrado);

    verificar(creado, "Crear arbol B+ vacio con raiz hoja");
    verificar(i1 && i2 && i3, "Guardar entradas ordenadas en hoja");
    verificar(existe, "Buscar clave existente en raiz hoja");
    verificar(encontrado.page_id == 20 && encontrado.slot_id == 1, "RID encontrado correcto");
    verificar(!no_existe, "Buscar clave inexistente devuelve falso");
}

void test_busqueda_con_nodo_interno(){
    std::remove("test_bplustree_internal.db");

    PageManager db("test_bplustree_internal.db");
    BufferManager bm(5, db);
    BPlusTree tree(bm, 1);

    Page* root = bm.fetchPage(1);
    BPlusTreeNode::init_internal(*root, 1);
    BPlusTreeNode::set_internal_key(*root, 0, 50);
    BPlusTreeNode::set_internal_child(*root, 0, 2);
    BPlusTreeNode::set_internal_child(*root, 1, 3);
    bm.unpinPage(1, true);
    bm.flushPage(1);

    Page* left = bm.fetchPage(2);
    BPlusTreeNode::init_leaf(*left, 2, 3, 1);
    BPlusTreeNode::set_leaf_entry(*left, 0, 10, {10, 0});
    BPlusTreeNode::set_leaf_entry(*left, 1, 40, {40, 1});
    bm.unpinPage(2, true);
    bm.flushPage(2);

    Page* right = bm.fetchPage(3);
    BPlusTreeNode::init_leaf(*right, 3, -1, 1);
    BPlusTreeNode::set_leaf_entry(*right, 0, 50, {50, 0});
    BPlusTreeNode::set_leaf_entry(*right, 1, 70, {70, 1});
    bm.unpinPage(3, true);
    bm.flushPage(3);

    RID encontrado = {-1, 0};
    bool existe_izquierda = tree.search(40, encontrado);
    bool rid_izquierda = encontrado.page_id == 40 && encontrado.slot_id == 1;

    bool existe_derecha = tree.search(70, encontrado);
    bool rid_derecha = encontrado.page_id == 70 && encontrado.slot_id == 1;

    verificar(existe_izquierda && rid_izquierda, "Buscar bajando al hijo izquierdo");
    verificar(existe_derecha && rid_derecha, "Buscar bajando al hijo derecho");
}

void test_indice_con_datos_de_alumnos(){
    std::remove("test_bplustree_alumnos.db");

    PageManager db("test_bplustree_alumnos.db");
    BufferManager bm(4, db);
    BPlusTree indice_alumnos(bm, 1);

    Page alumnos;
    SlottedPage::init(alumnos, 10);

    RID rid_ana;
    RID rid_luis;
    RID rid_maria;

    const char* ana = "1|Ana|20";
    const char* luis = "2|Luis|21";
    const char* maria = "3|Maria|22";

    bool insert_ana = SlottedPage::insert_record(alumnos, ana, std::strlen(ana), rid_ana);
    bool insert_luis = SlottedPage::insert_record(alumnos, luis, std::strlen(luis), rid_luis);
    bool insert_maria = SlottedPage::insert_record(alumnos, maria, std::strlen(maria), rid_maria);

    db.write_page(10, alumnos);
    db.sync();

    indice_alumnos.create_empty_tree();

    Page* raiz_indice = bm.fetchPage(1);
    bool idx_ana = BPlusTreeNode::set_leaf_entry(*raiz_indice, 0, 1, rid_ana);
    bool idx_luis = BPlusTreeNode::set_leaf_entry(*raiz_indice, 1, 2, rid_luis);
    bool idx_maria = BPlusTreeNode::set_leaf_entry(*raiz_indice, 2, 3, rid_maria);
    bm.unpinPage(1, true);
    bm.flushPage(1);

    RID rid_encontrado = {-1, 0};
    bool encontro_id_2 = indice_alumnos.search(2, rid_encontrado);

    Page pagina_datos;
    uint16_t size = 0;
    char salida[100];
    std::memset(salida, 0, sizeof(salida));

    bool leyo_pagina = db.read_page(rid_encontrado.page_id, pagina_datos);
    bool leyo_registro = SlottedPage::get_record(
        pagina_datos,
        rid_encontrado.slot_id,
        salida,
        size
    );

    if(leyo_registro)
        salida[size] = '\0';

    verificar(insert_ana && insert_luis && insert_maria, "Insertar alumnos en SlottedPage");
    verificar(idx_ana && idx_luis && idx_maria, "Crear indice B+ con id de alumno hacia RID");
    verificar(encontro_id_2, "Buscar id=2 usando el arbol B+");
    verificar(leyo_pagina && leyo_registro, "Usar el RID encontrado para leer el registro real");
    verificar(std::strcmp(salida, "2|Luis|21") == 0, "El indice encuentra el registro de Luis");
}

int main(){
    std::cout << "=== PRUEBA UNITARIA: B+ TREE ===\n\n";

    test_busqueda_en_raiz_hoja();
    test_busqueda_con_nodo_interno();
    test_indice_con_datos_de_alumnos();

    std::cout << "\nResumen:\n";
    std::cout << "Pruebas correctas: " << ok << "\n";
    std::cout << "Pruebas fallidas: " << fail << "\n";

    if(fail == 0){
        std::cout << "\nRESULTADO B+ TREE: OK\n";
        return 0;
    }

    std::cout << "\nRESULTADO B+ TREE: FALLO\n";
    return 1;
}
