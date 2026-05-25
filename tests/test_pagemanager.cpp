#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstring>
#include "../include/PageManager.h"

int ok=0;
int fail=0;

void verificar(bool condicion,const char* nombre){
    if(condicion){
        std::cout<<"[OK] "<<nombre<<"\n";
        ok++;
    }else{
        std::cout<<"[FAIL] "<<nombre<<"\n";
        fail++;
    }
}

bool existe_archivo(const char* nombre){
    std::ifstream f(nombre,std::ios::binary);
    return f.good();
}

void test_crear_archivo(){
    std::remove("test_pagemanager.db");

    PageManager db("test_pagemanager.db");

    verificar(db.is_open(),"Crear y abrir archivo de base de datos");
    verificar(existe_archivo("test_pagemanager.db"),"Verificar existencia del archivo .db");
}

void test_escribir_leer_pagina(){
    std::remove("test_pagemanager.db");

    PageManager db("test_pagemanager.db");

    Page p1;
    std::memset(&p1,0,sizeof(Page));

    p1.page_id=1;
    p1.next_page=-1;

    std::snprintf(
        p1.buffer,
        sizeof(p1.buffer),
        "Pagina escrita directamente con PageManager"
    );

    bool escrito=db.write_page(1,p1);
    bool sincronizado=db.sync();

    Page p2;
    std::memset(&p2,0,sizeof(Page));

    bool leido=db.read_page(1,p2);

    verificar(escrito,"Escribir pagina completa en disco");
    verificar(sincronizado,"Sincronizar archivo con fsync/FlushFileBuffers");
    verificar(leido,"Leer pagina completa desde disco");
    verificar(p2.page_id==1,"Recuperar page_id correcto");
    verificar(p2.next_page==-1,"Recuperar next_page correcto");
    verificar(
        std::strcmp(p2.buffer,"Pagina escrita directamente con PageManager")==0,
        "Recuperar contenido correcto"
    );
}

void test_integridad_byte_a_byte(){
    std::remove("test_pagemanager.db");

    PageManager db("test_pagemanager.db");

    Page original;
    std::memset(&original,0,sizeof(Page));

    original.page_id=2;
    original.next_page=99;

    for(size_t i=0;i<sizeof(original.buffer);i++)
        original.buffer[i]=static_cast<char>('A'+(i%26));

    bool escrito=db.write_page(2,original);
    bool sincronizado=db.sync();

    Page recuperada;
    std::memset(&recuperada,0,sizeof(Page));

    bool leido=db.read_page(2,recuperada);

    bool iguales=std::memcmp(&original,&recuperada,sizeof(Page))==0;

    verificar(escrito,"Escribir pagina para prueba byte a byte");
    verificar(sincronizado,"Sincronizar pagina byte a byte");
    verificar(leido,"Leer pagina para prueba byte a byte");
    verificar(iguales,"Integridad byte a byte de la pagina");
}

int main(){
    std::cout<<"=== PRUEBA UNITARIA: PAGEMANAGER ===\n\n";

    test_crear_archivo();
    test_escribir_leer_pagina();
    test_integridad_byte_a_byte();

    std::cout<<"\nResumen:\n";
    std::cout<<"Pruebas correctas: "<<ok<<"\n";
    std::cout<<"Pruebas fallidas: "<<fail<<"\n";

    if(fail==0){
        std::cout<<"\nRESULTADO PAGEMANAGER: OK\n";
        return 0;
    }

    std::cout<<"\nRESULTADO PAGEMANAGER: FALLO\n";
    return 1;
}