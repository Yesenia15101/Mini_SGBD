#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstring>
#include "../include/PageManager.h"
#include "../include/SlottedPage.h"
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

bool insertar_alumno(Page& page,int id,const char* nombre,int edad,RID& rid){
    char registro[200];

    std::snprintf(
        registro,
        sizeof(registro),
        "%d|%s|%d",
        id,
        nombre,
        edad
    );

    return SlottedPage::insert_record(
        page,
        registro,
        std::strlen(registro),
        rid
    );
}

bool consultar_por_slot(PageManager& db,int page_id,uint16_t slot_id,char* salida,int capacidad){
    Page page;

    if(!db.read_page(page_id,page))
        return false;

    uint16_t size=0;
    std::memset(salida,0,capacidad);

    bool encontrado=SlottedPage::get_record(page,slot_id,salida,size);

    if(!encontrado)
        return false;

    if(size>=capacidad)
        size=capacidad-1;

    salida[size]='\0';

    return true;
}

void consultar_todos(PageManager& db,int page_id){
    Page page;

    if(!db.read_page(page_id,page)){
        std::cout<<"No se pudo leer la pagina "<<page_id<<"\n";
        return;
    }

    std::cout<<"\nSELECT * FROM alumnos;\n";

    uint16_t total=SlottedPage::get_slot_count(page);

    for(uint16_t i=0;i<total;i++){
        char salida[200];
        uint16_t size=0;
        std::memset(salida,0,sizeof(salida));

        if(SlottedPage::get_record(page,i,salida,size)){
            salida[size]='\0';
            std::cout<<"slot "<<i<<" -> "<<salida<<"\n";
        }
    }
}

void test_integracion_con_insert_y_select(){
    const char* db_name="test_integration.db";
    const char* journal_name="test_integration.journal";

    std::remove(db_name);
    std::remove(journal_name);

    verificar(!existe_archivo(journal_name),"La prueba inicia sin journal pendiente");

    std::cout<<"\n=== CREACION E INSERCION DE DATOS ===\n";

    RID r1,r2,r3;

    {
        PageManager db(db_name);

        Page alumnos;
        SlottedPage::init(alumnos,10);

        std::cout<<"INSERT INTO alumnos VALUES (1,'Ana',20);\n";
        bool i1=insertar_alumno(alumnos,1,"Ana",20,r1);

        std::cout<<"INSERT INTO alumnos VALUES (2,'Luis',21);\n";
        bool i2=insertar_alumno(alumnos,2,"Luis",21,r2);

        std::cout<<"INSERT INTO alumnos VALUES (3,'Maria',22);\n";
        bool i3=insertar_alumno(alumnos,3,"Maria",22,r3);

        bool valida=SlottedPage::validate(alumnos);
        bool escrito=db.write_page(10,alumnos);
        bool sync=db.sync();

        verificar(db.is_open(),"Abrir archivo de base de datos");
        verificar(i1,"Insertar alumno Ana");
        verificar(i2,"Insertar alumno Luis");
        verificar(i3,"Insertar alumno Maria");
        verificar(r1.page_id==10 && r1.slot_id==0,"RID de Ana correcto");
        verificar(r2.page_id==10 && r2.slot_id==1,"RID de Luis correcto");
        verificar(r3.page_id==10 && r3.slot_id==2,"RID de Maria correcto");
        verificar(valida,"Validar estructura interna de la pagina");
        verificar(escrito,"Guardar pagina de alumnos en disco");
        verificar(sync,"Sincronizar base con fsync/FlushFileBuffers");
    }

    std::cout<<"\n=== CONSULTA DESPUES DE INSERTAR ===\n";

    {
        PageManager db(db_name);

        consultar_todos(db,10);

        char ana[200];
        char luis[200];
        char maria[200];

        bool c1=consultar_por_slot(db,10,0,ana,sizeof(ana));
        bool c2=consultar_por_slot(db,10,1,luis,sizeof(luis));
        bool c3=consultar_por_slot(db,10,2,maria,sizeof(maria));

        verificar(c1,"Consultar Ana por slot");
        verificar(c2,"Consultar Luis por slot");
        verificar(c3,"Consultar Maria por slot");
        verificar(std::strcmp(ana,"1|Ana|20")==0,"Ana consultada correctamente");
        verificar(std::strcmp(luis,"2|Luis|21")==0,"Luis consultado correctamente");
        verificar(std::strcmp(maria,"3|Maria|22")==0,"Maria consultada correctamente");
    }

    std::cout<<"\n=== VERIFICACION DE PERSISTENCIA ===\n";

    {
        PageManager db(db_name);

        char salida[200];

        bool persistio=consultar_por_slot(db,10,1,salida,sizeof(salida));

        verificar(persistio,"Reabrir base y consultar datos persistidos");
        verificar(std::strcmp(salida,"2|Luis|21")==0,"Los datos persisten despues de cerrar y reabrir");

        consultar_todos(db,10);
    }

    std::cout<<"\n=== MODIFICACION SIN COMMIT Y ROLLBACK ===\n";

    {
        PageManager db(db_name);
        JournalManager journal(journal_name);

        Page antes;

        bool leyo_original=db.read_page(10,antes);
        bool inicio=journal.begin();
        bool guardo=journal.save_page_before_change(10,antes);

        Page modificada;
        SlottedPage::init(modificada,10);

        RID r_mod;

        std::cout<<"UPDATE alumnos SET edad=99 WHERE id=1;  -- sin commit\n";
        bool insert_mod=insertar_alumno(modificada,1,"Ana",99,r_mod);

        bool escribio=db.write_page(10,modificada);
        bool sync=db.sync();

        char temporal[200];
        bool consulta_temporal=consultar_por_slot(db,10,0,temporal,sizeof(temporal));

        std::cout<<"\nConsulta antes del rollback:\n";
        consultar_todos(db,10);

        verificar(leyo_original,"Leer pagina original antes de modificar");
        verificar(inicio,"Iniciar rollback journal");
        verificar(guardo,"Guardar copia original en journal");
        verificar(insert_mod,"Crear version modificada sin commit");
        verificar(escribio,"Escribir modificacion sin commit");
        verificar(sync,"Sincronizar modificacion temporal");
        verificar(consulta_temporal,"Consultar version temporal");
        verificar(std::strcmp(temporal,"1|Ana|99")==0,"La base muestra cambio temporal antes del rollback");

        bool rollback=journal.rollback(db);

        char restaurada[200];
        bool consulta_restaurada=consultar_por_slot(db,10,0,restaurada,sizeof(restaurada));

        std::cout<<"\nConsulta despues del rollback:\n";
        consultar_todos(db,10);

        verificar(rollback,"Ejecutar rollback");
        verificar(consulta_restaurada,"Consultar despues del rollback");
        verificar(std::strcmp(restaurada,"1|Ana|20")==0,"Rollback restaura a Ana con edad original");
        verificar(!journal.exists(),"Journal eliminado despues del rollback");
    }

    std::cout<<"\n=== MODIFICACION CON COMMIT ===\n";

    {
        PageManager db(db_name);
        JournalManager journal(journal_name);

        Page antes;

        bool leyo_original=db.read_page(10,antes);
        bool inicio=journal.begin();
        bool guardo=journal.save_page_before_change(10,antes);

        Page confirmada;
        SlottedPage::init(confirmada,10);

        RID r1c,r2c,r3c;

        std::cout<<"UPDATE alumnos SET edad=23 WHERE id=3;  -- con commit\n";

        bool i1=insertar_alumno(confirmada,1,"Ana",20,r1c);
        bool i2=insertar_alumno(confirmada,2,"Luis",21,r2c);
        bool i3=insertar_alumno(confirmada,3,"Maria",23,r3c);

        bool escribio=db.write_page(10,confirmada);
        bool sync=db.sync();
        bool commit=journal.commit();

        verificar(leyo_original,"Leer pagina antes del commit");
        verificar(inicio,"Iniciar journal para commit");
        verificar(guardo,"Guardar copia original antes del commit");
        verificar(i1 && i2 && i3,"Crear nueva version confirmada de alumnos");
        verificar(escribio,"Escribir cambios confirmados en disco");
        verificar(sync,"Sincronizar cambios confirmados");
        verificar(commit,"Confirmar commit y eliminar journal");
        verificar(!journal.exists(),"No queda journal despues del commit");
    }

    std::cout<<"\n=== CONSULTA FINAL DESPUES DEL COMMIT ===\n";

    {
        PageManager db(db_name);

        consultar_todos(db,10);

        char maria[200];

        bool consulta_final=consultar_por_slot(db,10,2,maria,sizeof(maria));

        verificar(consulta_final,"Consultar Maria despues del commit");
        verificar(std::strcmp(maria,"3|Maria|23")==0,"Commit persiste la nueva edad de Maria");
    }
}

int main(){
    std::cout<<"=== PRUEBA DE INTEGRACION: STORAGE MANAGER ===\n";
    std::cout<<"Caso: ingresar alumnos, consultar, probar rollback y commit\n";

    test_integracion_con_insert_y_select();

    std::cout<<"\nResumen:\n";
    std::cout<<"Pruebas correctas: "<<ok<<"\n";
    std::cout<<"Pruebas fallidas: "<<fail<<"\n";

    if(fail==0){
        std::cout<<"\nRESULTADO INTEGRACION STORAGE MANAGER: OK\n";
        return 0;
    }

    std::cout<<"\nRESULTADO INTEGRACION STORAGE MANAGER: FALLO\n";
    return 1;
}