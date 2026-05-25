#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstring>
#include "../include/PageManager.h"
#include "../include/JournalManager.h"

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

void preparar_pagina(Page& p,int page_id,const char* texto){
    std::memset(&p,0,sizeof(Page));
    p.page_id=page_id;
    p.next_page=-1;
    std::snprintf(p.buffer,sizeof(p.buffer),"%s",texto);
}

void test_begin_save_commit(){
    std::remove("test_journal.db");
    std::remove("test_journal.journal");

    PageManager db("test_journal.db");
    JournalManager journal("test_journal.journal");

    Page original;
    preparar_pagina(original,1,"VERSION ORIGINAL");

    db.write_page(1,original);
    db.sync();

    bool inicio=journal.begin();
    bool existe_despues_begin=journal.exists();

    bool guardo=journal.save_page_before_change(1,original);
    bool existe_despues_guardar=journal.exists();

    bool commit=journal.commit();
    bool eliminado=!journal.exists();

    verificar(inicio,"Iniciar journal");
    verificar(existe_despues_begin,"Journal existe después de begin");
    verificar(guardo,"Guardar página original en journal");
    verificar(existe_despues_guardar,"Journal existe después de guardar página");
    verificar(commit,"Ejecutar commit del journal");
    verificar(eliminado,"Journal eliminado después del commit");
}

void test_rollback_restaura_pagina(){
    std::remove("test_journal.db");
    std::remove("test_journal.journal");

    PageManager db("test_journal.db");
    JournalManager journal("test_journal.journal");

    Page original;
    preparar_pagina(original,2,"VERSION ORIGINAL");

    db.write_page(2,original);
    db.sync();

    Page antes;
    bool leyo_original=db.read_page(2,antes);

    bool inicio=journal.begin();
    bool guardo=journal.save_page_before_change(2,antes);

    Page modificada;
    preparar_pagina(modificada,2,"VERSION MODIFICADA SIN COMMIT");

    bool escribio_modificada=db.write_page(2,modificada);
    bool sync_modificada=db.sync();

    Page comprobacion_modificada;
    db.read_page(2,comprobacion_modificada);

    bool cambio_aplicado=std::strcmp(
        comprobacion_modificada.buffer,
        "VERSION MODIFICADA SIN COMMIT"
    )==0;

    bool rollback=journal.rollback(db);

    Page recuperada;
    bool leyo_recuperada=db.read_page(2,recuperada);

    bool restaurada=std::strcmp(
        recuperada.buffer,
        "VERSION ORIGINAL"
    )==0;

    bool journal_eliminado=!journal.exists();

    verificar(leyo_original,"Leer página original antes de modificar");
    verificar(inicio,"Iniciar journal para rollback");
    verificar(guardo,"Guardar copia original antes del cambio");
    verificar(escribio_modificada,"Escribir página modificada en la base");
    verificar(sync_modificada,"Sincronizar página modificada");
    verificar(cambio_aplicado,"Confirmar que la base quedó modificada antes del rollback");
    verificar(rollback,"Ejecutar rollback");
    verificar(leyo_recuperada,"Leer página después del rollback");
    verificar(restaurada,"Rollback restaura la versión original");
    verificar(journal_eliminado,"Journal eliminado después del rollback");
}

int main(){
    std::cout<<"=== PRUEBA UNITARIA: JOURNALMANAGER ===\n\n";

    test_begin_save_commit();
    test_rollback_restaura_pagina();

    std::cout<<"\nResumen:\n";
    std::cout<<"Pruebas correctas: "<<ok<<"\n";
    std::cout<<"Pruebas fallidas: "<<fail<<"\n";

    if(fail==0){
        std::cout<<"\nRESULTADO JOURNALMANAGER: OK\n";
        return 0;
    }

    std::cout<<"\nRESULTADO JOURNALMANAGER: FALLO\n";
    return 1;
}