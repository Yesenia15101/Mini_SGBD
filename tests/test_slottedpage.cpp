#include <iostream>
#include <cstring>
#include "../include/SlottedPage.h"

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

void test_init_page(){
    Page p;
    SlottedPage::init(p,1);

    verificar(p.page_id==1,"Inicializar page_id");
    verificar(p.next_page==-1,"Inicializar next_page");
    verificar(SlottedPage::is_initialized(p),"Página inicializada con magic válido");
    verificar(SlottedPage::get_slot_count(p)==0,"Página inicia con 0 slots");
    verificar(SlottedPage::validate(p),"Página inicial válida");
}

void test_insert_get_record(){
    Page p;
    SlottedPage::init(p,2);

    RID rid;
    const char* registro="1|Ana|20";

    bool insertado=SlottedPage::insert_record(p,registro,std::strlen(registro),rid);

    char salida[100];
    uint16_t size=0;
    std::memset(salida,0,sizeof(salida));

    bool recuperado=SlottedPage::get_record(p,rid.slot_id,salida,size);
    salida[size]='\0';

    verificar(insertado,"Insertar un registro");
    verificar(rid.page_id==2,"RID guarda page_id correcto");
    verificar(rid.slot_id==0,"RID guarda slot_id correcto");
    verificar(recuperado,"Recuperar registro por slot");
    verificar(std::strcmp(salida,registro)==0,"Registro recuperado igual al original");
    verificar(SlottedPage::get_slot_count(p)==1,"Slot count incrementado a 1");
    verificar(SlottedPage::validate(p),"Página válida después de insertar");
}

void test_insert_varios_registros(){
    Page p;
    SlottedPage::init(p,3);

    RID r1,r2,r3;

    const char* a="1|Ana|20";
    const char* b="2|Luis Alberto|21";
    const char* c="3|Maria Fernanda|22";

    bool i1=SlottedPage::insert_record(p,a,std::strlen(a),r1);
    bool i2=SlottedPage::insert_record(p,b,std::strlen(b),r2);
    bool i3=SlottedPage::insert_record(p,c,std::strlen(c),r3);

    char salida[100];
    uint16_t size=0;
    std::memset(salida,0,sizeof(salida));

    bool g2=SlottedPage::get_record(p,r2.slot_id,salida,size);
    salida[size]='\0';

    verificar(i1 && i2 && i3,"Insertar varios registros");
    verificar(SlottedPage::get_slot_count(p)==3,"Slot count igual a 3");
    verificar(g2,"Recuperar registro intermedio");
    verificar(std::strcmp(salida,b)==0,"Registro intermedio correcto");
    verificar(SlottedPage::validate(p),"Página válida con varios registros");
}

void test_delete_record(){
    Page p;
    SlottedPage::init(p,4);

    RID r1,r2;

    const char* a="1|Ana";
    const char* b="2|Luis";

    SlottedPage::insert_record(p,a,std::strlen(a),r1);
    SlottedPage::insert_record(p,b,std::strlen(b),r2);

    bool eliminado=SlottedPage::delete_record(p,r1.slot_id);

    char salida[100];
    uint16_t size=0;
    std::memset(salida,0,sizeof(salida));

    bool recuperado=SlottedPage::get_record(p,r1.slot_id,salida,size);

    verificar(eliminado,"Eliminar registro lógicamente");
    verificar(!recuperado,"No recuperar registro eliminado");
    verificar(SlottedPage::validate(p),"Página válida después de eliminar");
}

void test_reuse_deleted_slot(){
    Page p;
    SlottedPage::init(p,5);

    RID r1,r2,r3;

    const char* a="1|Ana";
    const char* b="2|Luis";
    const char* c="3|Carlos";

    SlottedPage::insert_record(p,a,std::strlen(a),r1);
    SlottedPage::insert_record(p,b,std::strlen(b),r2);

    SlottedPage::delete_record(p,r1.slot_id);

    bool insertado=SlottedPage::insert_record(p,c,std::strlen(c),r3);

    char salida[100];
    uint16_t size=0;
    std::memset(salida,0,sizeof(salida));

    bool recuperado=SlottedPage::get_record(p,r3.slot_id,salida,size);
    salida[size]='\0';

    verificar(insertado,"Insertar después de eliminar");
    verificar(r3.slot_id==r1.slot_id,"Reutilizar slot eliminado");
    verificar(recuperado,"Recuperar nuevo registro");
    verificar(std::strcmp(salida,c)==0,"Nuevo registro correcto");
    verificar(SlottedPage::get_slot_count(p)==2,"Slot count no aumenta al reutilizar slot");
    verificar(SlottedPage::validate(p),"Página válida después de reutilizar slot");
}

int main(){
    std::cout<<"=== PRUEBA UNITARIA: SLOTTEDPAGE ===\n\n";

    test_init_page();
    test_insert_get_record();
    test_insert_varios_registros();
    test_delete_record();
    test_reuse_deleted_slot();

    std::cout<<"\nResumen:\n";
    std::cout<<"Pruebas correctas: "<<ok<<"\n";
    std::cout<<"Pruebas fallidas: "<<fail<<"\n";

    if(fail==0){
        std::cout<<"\nRESULTADO SLOTTEDPAGE: OK\n";
        return 0;
    }

    std::cout<<"\nRESULTADO SLOTTEDPAGE: FALLO\n";
    return 1;
}