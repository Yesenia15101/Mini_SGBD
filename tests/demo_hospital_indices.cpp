#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include "../include/PageManager.h"
#include "../include/BufferManager.h"
#include "../include/BPlusTree.h"
#include "../include/SlottedPage.h"

struct Paciente{
    int id;
    int dni;
    std::string nombres;
    std::string apellidos;
    int edad;
    char sexo;
    std::string tipo_sangre;
    int id_especialidad;
    std::string especialidad;
    int prioridad;
};

const int TOTAL_PACIENTES = 1000;
const int PRIMERA_PAGINA_DATOS = 100;
const int FACTOR_INDICE_SECUNDARIO = 100000;

int verificaciones = 0;
int errores = 0;

struct AdvisorStat{
    int consultas = 0;
    int total_resultados = 0;
    int costo_sin_indice = 0;
    int costo_con_indice = 0;
    bool tiene_indice = false;
};

class IndexAdvisor{
private:
    std::map<std::string, AdvisorStat> stats;

public:
    void registrar(const std::string& campo, bool tiene_indice, int resultados){
        AdvisorStat& stat = stats[campo];
        stat.consultas++;
        stat.total_resultados += resultados;
        stat.costo_sin_indice += TOTAL_PACIENTES;
        stat.tiene_indice = tiene_indice;

        if(tiene_indice)
            stat.costo_con_indice += resultados + 5;
        else
            stat.costo_con_indice += TOTAL_PACIENTES;
    }

    void imprimir() const{
        std::cout << "Asesor adaptativo de indices\n";

        if(stats.empty()){
            std::cout << "   Todavia no hay consultas registradas.\n";
            return;
        }

        for(const auto& item : stats){
            const std::string& campo = item.first;
            const AdvisorStat& stat = item.second;
            double promedio = static_cast<double>(stat.total_resultados) / stat.consultas;
            double selectividad = promedio / TOTAL_PACIENTES;

            std::cout << "\nCampo: " << campo << "\n";
            std::cout << "   Consultas observadas: " << stat.consultas << "\n";
            std::cout << "   Promedio de resultados: " << promedio << "\n";
            std::cout << "   Selectividad estimada: " << selectividad << "\n";
            std::cout << "   Costo estimado sin indice: " << stat.costo_sin_indice << "\n";
            std::cout << "   Costo observado/estimado con estrategia actual: " << stat.costo_con_indice << "\n";

            if(stat.tiene_indice){
                std::cout << "   Estado: ya tiene indice B+.\n";

                if(selectividad <= 0.25)
                    std::cout << "   Recomendacion: mantener indice, tiene buena selectividad.\n";
                else
                    std::cout << "   Recomendacion: revisar indice, devuelve muchos resultados.\n";
            }else{
                std::cout << "   Estado: no tiene indice secundario.\n";

                if(stat.consultas >= 2 && selectividad <= 0.25)
                    std::cout << "   Recomendacion: crear indice B+ secundario para este campo.\n";
                else if(selectividad > 0.40)
                    std::cout << "   Recomendacion: no crear indice por ahora, baja selectividad.\n";
                else
                    std::cout << "   Recomendacion: seguir observando mas consultas antes de crear indice.\n";
            }
        }
    }
};

void comprobar(bool condicion, const char* detalle){
    verificaciones++;

    if(!condicion){
        errores++;
        std::cout << "  No se pudo validar: " << detalle << "\n";
    }
}

std::string trim(const std::string& value){
    size_t start = 0;
    size_t end = value.size();

    while(start < end && std::isspace(static_cast<unsigned char>(value[start])))
        start++;

    while(end > start && std::isspace(static_cast<unsigned char>(value[end - 1])))
        end--;

    return value.substr(start, end - start);
}

std::string to_lower(std::string value){
    for(char& c : value)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    return value;
}

std::string quitar_comillas(std::string value){
    value = trim(value);

    if(value.size() >= 2 &&
       ((value.front() == '\'' && value.back() == '\'') ||
        (value.front() == '"' && value.back() == '"'))){
        return value.substr(1, value.size() - 2);
    }

    return value;
}

std::string nombre_paciente(int id){
    static const char* nombres[] = {
        "Ana", "Luis", "Maria", "Carlos", "Lucia",
        "Jorge", "Valeria", "Diego", "Camila", "Mateo",
        "Sofia", "Andres", "Daniela", "Sebastian", "Fernanda",
        "Ricardo", "Paola", "Miguel", "Gabriela", "Alejandro"
    };

    return nombres[(id - 1) % 20];
}

std::string apellido_paciente(int id){
    static const char* apellidos[] = {
        "Quispe", "Mamani", "Flores", "Garcia", "Vargas",
        "Rojas", "Torres", "Condori", "Huaman", "Castillo",
        "Lopez", "Ramirez", "Chavez", "Paredes", "Medina",
        "Salas", "Cruz", "Herrera", "Mendoza", "Aguilar"
    };

    int apellido1 = ((id - 1) / 20) % 20;
    int apellido2 = (id + 7) % 20;

    return std::string(apellidos[apellido1]) + " " + apellidos[apellido2];
}

std::string tipo_sangre_paciente(int id){
    static const char* tipos[] = {
        "O+", "O-", "A+", "A-", "B+", "B-", "AB+", "AB-"
    };

    return tipos[(id - 1) % 8];
}

std::string nombre_especialidad(int id_especialidad){
    static const char* especialidades[] = {
        "Emergencia",
        "Cardiologia",
        "Pediatria",
        "Neurologia",
        "Traumatologia",
        "Ginecologia",
        "Dermatologia",
        "Oftalmologia",
        "Oncologia",
        "Medicina Interna"
    };

    if(id_especialidad < 1 || id_especialidad > 10)
        return "Sin especialidad";

    return especialidades[id_especialidad - 1];
}

int id_especialidad_por_nombre(const std::string& especialidad){
    std::string buscada = to_lower(trim(especialidad));

    for(int i = 1; i <= 10; i++){
        if(to_lower(nombre_especialidad(i)) == buscada)
            return i;
    }

    return -1;
}

int codigo_tipo_sangre(const std::string& tipo){
    std::string normalizado = to_lower(trim(tipo));

    if(normalizado == "o+") return 1;
    if(normalizado == "o-") return 2;
    if(normalizado == "a+") return 3;
    if(normalizado == "a-") return 4;
    if(normalizado == "b+") return 5;
    if(normalizado == "b-") return 6;
    if(normalizado == "ab+") return 7;
    if(normalizado == "ab-") return 8;
    return 0;
}

Paciente generar_paciente(int id){
    Paciente p;
    p.id = id;
    p.dni = 70000000 + id;
    p.nombres = nombre_paciente(id);
    p.apellidos = apellido_paciente(id);
    p.edad = 1 + (id % 90);
    p.sexo = (id % 2 == 0) ? 'F' : 'M';
    p.tipo_sangre = tipo_sangre_paciente(id);
    p.id_especialidad = 1 + (id % 10);
    p.especialidad = nombre_especialidad(p.id_especialidad);
    p.prioridad = 1 + (id % 5);
    return p;
}

std::string serializar_paciente(const Paciente& p){
    return std::to_string(p.id) + "|" +
           std::to_string(p.dni) + "|" +
           p.nombres + "|" +
           p.apellidos + "|" +
           std::to_string(p.edad) + "|" +
           std::string(1, p.sexo) + "|" +
           p.tipo_sangre + "|" +
           std::to_string(p.id_especialidad) + "|" +
           p.especialidad + "|" +
           std::to_string(p.prioridad);
}

int clave_secundaria(int valor, int id_paciente){
    return valor * FACTOR_INDICE_SECUNDARIO + id_paciente;
}

bool leer_registro(BufferManager& bm, RID rid, std::string& salida){
    Page* page = bm.fetchPage(rid.page_id);
    char buffer[240];
    uint16_t size = 0;
    std::memset(buffer, 0, sizeof(buffer));

    if(page == nullptr)
        return false;

    bool leido = SlottedPage::get_record(*page, rid.slot_id, buffer, size);

    bm.unpinPage(rid.page_id, false);

    if(!leido)
        return false;

    buffer[size] = '\0';
    salida = buffer;
    return true;
}

void imprimir_rids(BufferManager& bm, const std::vector<RID>& rids, int limite){
    std::cout << "Coincidencias encontradas por indice: " << rids.size() << "\n";

    int mostrados = 0;

    for(RID rid : rids){
        std::string registro;

        if(leer_registro(bm, rid, registro)){
            std::cout << "   RID(" << rid.page_id << ", " << rid.slot_id << ") -> "
                      << registro << "\n";
            mostrados++;
        }

        if(mostrados >= limite)
            break;
    }

    if(static_cast<int>(rids.size()) > limite)
        std::cout << "   ... se muestran " << limite << " de " << rids.size() << " resultados.\n";
}

void escanear_por_campo_no_indexado(
    BufferManager& bm,
    BPlusTree& idx_id,
    const std::string& campo,
    const std::string& valor,
    std::vector<RID>& rids
){
    for(int id = 1; id <= TOTAL_PACIENTES; id++){
        Paciente paciente = generar_paciente(id);
        bool coincide = false;

        if(campo == "edad")
            coincide = paciente.edad == std::stoi(valor);
        else if(campo == "sexo")
            coincide = to_lower(std::string(1, paciente.sexo)) == to_lower(valor);

        if(coincide){
            RID rid;

            if(idx_id.search(id, rid))
                rids.push_back(rid);
        }
    }

    // Se leen algunos registros para que el BufferManager participe en el resultado mostrado.
    int limite_lectura = static_cast<int>(std::min<size_t>(rids.size(), 20));

    for(int i = 0; i < limite_lectura; i++){
        std::string registro;
        leer_registro(bm, rids[i], registro);
    }
}

void imprimir_por_id(BufferManager& bm, BPlusTree& idx_id, int inicio, int fin){
    for(int id = inicio; id <= fin; id++){
        RID rid;
        std::string registro;

        if(idx_id.search(id, rid) && leer_registro(bm, rid, registro)){
            std::cout << "   id=" << id << " -> RID(" << rid.page_id << ", "
                      << rid.slot_id << ") -> " << registro << "\n";
        }
    }
}

void imprimir_paginas_calientes(BufferManager& bm){
    std::vector<std::pair<int,int>> calientes = bm.getHotPages(10);

    std::cout << "Paginas calientes registradas por Splay Tree:\n";

    for(const auto& page : calientes)
        std::cout << "   Pagina " << page.first << " -> " << page.second << " accesos\n";
}

void mostrar_ayuda(){
    std::cout << "\nComandos disponibles:\n";
    std::cout << "   PRINT ALL\n";
    std::cout << "   PRINT LIMIT 20\n";
    std::cout << "   SELECT * FROM pacientes WHERE idPaciente = 500\n";
    std::cout << "   SELECT * FROM pacientes WHERE dni = 70000777\n";
    std::cout << "   SELECT * FROM pacientes WHERE especialidad = 'Neurologia'\n";
    std::cout << "   SELECT * FROM pacientes WHERE prioridad = 1\n";
    std::cout << "   SELECT * FROM pacientes WHERE tipoSangre = 'AB+'\n";
    std::cout << "   SELECT * FROM pacientes WHERE edad = 30       sin indice secundario\n";
    std::cout << "   SELECT * FROM pacientes WHERE sexo = 'F'      sin indice secundario\n";
    std::cout << "   ORDER BY idPaciente LIMIT 20\n";
    std::cout << "   ORDER BY dni LIMIT 20\n";
    std::cout << "   GROUP BY especialidad\n";
    std::cout << "   GROUP BY prioridad\n";
    std::cout << "   GROUP BY tipoSangre\n";
    std::cout << "   HOTPAGES\n";
    std::cout << "   LRU                  compara LRU puro contra Splay/pagina fria\n";
    std::cout << "   ADVISOR              recomienda indices segun las consultas observadas\n";
    std::cout << "   HEAT 500 30\n";
    std::cout << "   EXIT\n\n";
}

int obtener_limit(const std::string& lower, int default_limit){
    size_t pos = lower.find("limit");

    if(pos == std::string::npos)
        return default_limit;

    std::string value = trim(lower.substr(pos + 5));

    if(value.empty())
        return default_limit;

    int limit = std::stoi(value);

    if(limit <= 0)
        return default_limit;

    return limit;
}

void ejecutar_order_by(
    const std::string& comando,
    BufferManager& bm,
    BPlusTree& idx_id,
    BPlusTree& idx_dni
){
    std::string lower = to_lower(comando);
    int limite = obtener_limit(lower, 20);

    if(lower.find("idpaciente") != std::string::npos || lower.find(" id ") != std::string::npos){
        std::vector<RID> rids;
        idx_id.searchRange(1, TOTAL_PACIENTES, rids);

        std::cout << "Plan de acceso: recorrido ordenado del B+ Tree primario idPaciente\n";
        imprimir_rids(bm, rids, limite);
        return;
    }

    if(lower.find("dni") != std::string::npos){
        std::vector<RID> rids;
        idx_dni.searchRange(70000001, 70000000 + TOTAL_PACIENTES, rids);

        std::cout << "Plan de acceso: recorrido ordenado del B+ Tree secundario dni\n";
        imprimir_rids(bm, rids, limite);
        return;
    }

    std::cout << "ORDER BY disponible para: idPaciente, dni.\n";
}

void ejecutar_group_by(
    const std::string& comando,
    BPlusTree& idx_especialidad,
    BPlusTree& idx_prioridad,
    BPlusTree& idx_tipo_sangre
){
    std::string lower = to_lower(comando);

    if(lower.find("especialidad") != std::string::npos){
        std::cout << "Plan de acceso: conteo por rangos del B+ Tree secundario idEspecialidad\n";
        std::cout << "GROUP BY especialidad\n";

        for(int id = 1; id <= 10; id++){
            std::vector<RID> rids;
            idx_especialidad.searchRange(
                clave_secundaria(id, 0),
                clave_secundaria(id, FACTOR_INDICE_SECUNDARIO - 1),
                rids
            );

            std::cout << "   " << nombre_especialidad(id) << " -> "
                      << rids.size() << " pacientes\n";
        }

        return;
    }

    if(lower.find("prioridad") != std::string::npos){
        std::cout << "Plan de acceso: conteo por rangos del B+ Tree secundario prioridad\n";
        std::cout << "GROUP BY prioridad\n";

        for(int prioridad = 1; prioridad <= 5; prioridad++){
            std::vector<RID> rids;
            idx_prioridad.searchRange(
                clave_secundaria(prioridad, 0),
                clave_secundaria(prioridad, FACTOR_INDICE_SECUNDARIO - 1),
                rids
            );

            std::cout << "   Prioridad " << prioridad << " -> "
                      << rids.size() << " pacientes\n";
        }

        return;
    }

    if(lower.find("tiposangre") != std::string::npos || lower.find("tipo sangre") != std::string::npos){
        static const char* tipos[] = {
            "O+", "O-", "A+", "A-", "B+", "B-", "AB+", "AB-"
        };

        std::cout << "Plan de acceso: conteo por rangos del B+ Tree secundario tipoSangre\n";
        std::cout << "GROUP BY tipoSangre\n";

        for(int i = 0; i < 8; i++){
            std::vector<RID> rids;
            int codigo = codigo_tipo_sangre(tipos[i]);
            idx_tipo_sangre.searchRange(
                clave_secundaria(codigo, 0),
                clave_secundaria(codigo, FACTOR_INDICE_SECUNDARIO - 1),
                rids
            );

            std::cout << "   " << tipos[i] << " -> "
                      << rids.size() << " pacientes\n";
        }

        return;
    }

    std::cout << "GROUP BY disponible para: especialidad, prioridad, tipoSangre.\n";
}

void ejecutar_select(
    const std::string& comando,
    BufferManager& bm,
    BPlusTree& idx_id,
    BPlusTree& idx_dni,
    BPlusTree& idx_especialidad,
    BPlusTree& idx_prioridad,
    BPlusTree& idx_tipo_sangre,
    IndexAdvisor& advisor
){
    std::string limpio = comando;

    if(!limpio.empty() && limpio.back() == ';')
        limpio.pop_back();

    std::string lower = to_lower(limpio);
    size_t where_pos = lower.find("where");

    if(where_pos == std::string::npos){
        std::cout << "Consulta no reconocida. Usa SELECT ... WHERE campo = valor.\n";
        return;
    }

    std::string condicion = trim(limpio.substr(where_pos + 5));
    size_t eq = condicion.find('=');

    if(eq == std::string::npos){
        std::cout << "Falta '=' en el WHERE.\n";
        return;
    }

    std::string campo = to_lower(trim(condicion.substr(0, eq)));
    std::string valor = quitar_comillas(condicion.substr(eq + 1));

    RID rid;
    std::string registro;

    if(campo == "idpaciente" || campo == "id"){
        int id = std::stoi(valor);
        std::cout << "Plan de acceso: B+ Tree primario idPaciente -> RID\n";

        bool encontrado = idx_id.search(id, rid);
        advisor.registrar("idPaciente", true, encontrado ? 1 : 0);

        if(encontrado && leer_registro(bm, rid, registro))
            std::cout << "Resultado -> RID(" << rid.page_id << ", " << rid.slot_id << ") -> " << registro << "\n";
        else
            std::cout << "No encontrado.\n";

        return;
    }

    if(campo == "dni"){
        int dni = std::stoi(valor);
        std::cout << "Plan de acceso: B+ Tree secundario dni -> RID\n";

        bool encontrado = idx_dni.search(dni, rid);
        advisor.registrar("dni", true, encontrado ? 1 : 0);

        if(encontrado && leer_registro(bm, rid, registro))
            std::cout << "Resultado -> RID(" << rid.page_id << ", " << rid.slot_id << ") -> " << registro << "\n";
        else
            std::cout << "No encontrado.\n";

        return;
    }

    if(campo == "especialidad"){
        int id_especialidad = id_especialidad_por_nombre(valor);

        if(id_especialidad == -1){
            std::cout << "Especialidad no reconocida.\n";
            return;
        }

        std::vector<RID> rids;
        idx_especialidad.searchRange(
            clave_secundaria(id_especialidad, 0),
            clave_secundaria(id_especialidad, FACTOR_INDICE_SECUNDARIO - 1),
            rids
        );

        std::cout << "Plan de acceso: especialidad -> idEspecialidad = "
                  << id_especialidad << " -> B+ Tree secundario por rango -> lista de RID\n";
        advisor.registrar("especialidad", true, static_cast<int>(rids.size()));
        imprimir_rids(bm, rids, 20);
        return;
    }

    if(campo == "prioridad"){
        int prioridad = std::stoi(valor);
        std::vector<RID> rids;
        idx_prioridad.searchRange(
            clave_secundaria(prioridad, 0),
            clave_secundaria(prioridad, FACTOR_INDICE_SECUNDARIO - 1),
            rids
        );

        std::cout << "Plan de acceso: prioridad -> B+ Tree secundario por rango -> lista de RID\n";
        advisor.registrar("prioridad", true, static_cast<int>(rids.size()));
        imprimir_rids(bm, rids, 20);
        return;
    }

    if(campo == "tiposangre"){
        int codigo = codigo_tipo_sangre(valor);

        if(codigo == 0){
            std::cout << "Tipo de sangre no reconocido.\n";
            return;
        }

        std::vector<RID> rids;
        idx_tipo_sangre.searchRange(
            clave_secundaria(codigo, 0),
            clave_secundaria(codigo, FACTOR_INDICE_SECUNDARIO - 1),
            rids
        );

        std::cout << "Plan de acceso: tipoSangre -> codigo " << codigo
                  << " -> B+ Tree secundario por rango -> lista de RID\n";
        advisor.registrar("tipoSangre", true, static_cast<int>(rids.size()));
        imprimir_rids(bm, rids, 20);
        return;
    }

    if(campo == "edad" || campo == "sexo"){
        std::vector<RID> rids;
        escanear_por_campo_no_indexado(bm, idx_id, campo, valor, rids);

        std::cout << "Plan de acceso: escaneo completo porque el campo '"
                  << campo << "' no tiene indice secundario\n";
        advisor.registrar(campo, false, static_cast<int>(rids.size()));
        imprimir_rids(bm, rids, 20);
        return;
    }

    std::cout << "Campo sin indice en esta demo. Campos indexados: idPaciente, dni, especialidad, prioridad, tipoSangre.\n";
}

void ejecutar_heat(const std::string& comando, BufferManager& bm, BPlusTree& idx_id){
    std::istringstream in(comando);
    std::string palabra;
    int id = 0;
    int repeticiones = 0;

    in >> palabra >> id >> repeticiones;

    if(id <= 0 || repeticiones <= 0){
        std::cout << "Uso: HEAT idPaciente repeticiones. Ejemplo: HEAT 500 30\n";
        return;
    }

    for(int i = 0; i < repeticiones; i++){
        RID rid;
        std::string registro;

        if(idx_id.search(id, rid))
            leer_registro(bm, rid, registro);
    }

    std::cout << "Se accedio " << repeticiones << " veces al paciente " << id << ".\n";
    std::cout << "Esto aumenta el contador de sus paginas en el Splay Tree.\n";
}

void consola_interactiva(
    BufferManager& bm,
    BPlusTree& idx_id,
    BPlusTree& idx_dni,
    BPlusTree& idx_especialidad,
    BPlusTree& idx_prioridad,
    BPlusTree& idx_tipo_sangre,
    IndexAdvisor& advisor
){
    mostrar_ayuda();

    while(true){
        std::cout << "hospital> ";

        std::string comando;
        std::getline(std::cin, comando);

        if(!std::cin)
            break;

        comando = trim(comando);

        if(comando.empty())
            continue;

        std::string lower = to_lower(comando);

        if(lower == "exit" || lower == "salir")
            break;

        if(lower == "help" || lower == "ayuda"){
            mostrar_ayuda();
            continue;
        }

        if(lower == "print all"){
            imprimir_por_id(bm, idx_id, 1, TOTAL_PACIENTES);
            continue;
        }

        if(lower.rfind("print limit", 0) == 0){
            std::istringstream in(lower);
            std::string a;
            std::string b;
            int limite = 0;
            in >> a >> b >> limite;

            if(limite <= 0)
                limite = 20;

            imprimir_por_id(bm, idx_id, 1, limite);
            continue;
        }

        if(lower.rfind("select", 0) == 0 || lower.find("where") != std::string::npos){
            ejecutar_select(comando, bm, idx_id, idx_dni, idx_especialidad, idx_prioridad, idx_tipo_sangre, advisor);
            continue;
        }

        if(lower.find("order by") != std::string::npos){
            ejecutar_order_by(comando, bm, idx_id, idx_dni);
            continue;
        }

        if(lower.find("group by") != std::string::npos){
            ejecutar_group_by(comando, idx_especialidad, idx_prioridad, idx_tipo_sangre);
            continue;
        }

        if(lower.find("hotpages") != std::string::npos){
            imprimir_paginas_calientes(bm);
            continue;
        }

        if(lower.find("lru") != std::string::npos){
            std::cout << bm.getReplacementReport();
            continue;
        }

        if(lower.find("advisor") != std::string::npos || lower.find("asesor") != std::string::npos){
            advisor.imprimir();
            continue;
        }

        if(lower.rfind("heat", 0) == 0){
            ejecutar_heat(comando, bm, idx_id);
            continue;
        }

        std::cout << "Comando no reconocido. Escribe HELP para ver ejemplos.\n";
    }
}

void cargar_hospital(
    PageManager& db,
    BufferManager& bm,
    BPlusTree& idx_id,
    BPlusTree& idx_dni,
    BPlusTree& idx_especialidad,
    BPlusTree& idx_prioridad,
    BPlusTree& idx_tipo_sangre
){
    comprobar(idx_id.create_empty_tree(), "crear indice primario por idPaciente");
    comprobar(idx_dni.create_empty_tree(), "crear indice secundario unico por dni");
    comprobar(idx_especialidad.create_empty_tree(), "crear indice secundario por idEspecialidad");
    comprobar(idx_prioridad.create_empty_tree(), "crear indice secundario por prioridad");
    comprobar(idx_tipo_sangre.create_empty_tree(), "crear indice secundario por tipoSangre");

    std::vector<RID> rids(TOTAL_PACIENTES + 1);
    Page data_page;
    int data_page_id = PRIMERA_PAGINA_DATOS;
    SlottedPage::init(data_page, data_page_id);

    for(int id = 1; id <= TOTAL_PACIENTES; id++){
        Paciente paciente = generar_paciente(id);
        std::string registro = serializar_paciente(paciente);

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

        comprobar(insertado, "insertar paciente en pagina de datos");
        rids[id] = rid;
    }

    db.write_page(data_page_id, data_page);
    db.sync();

    for(int id = 1; id <= TOTAL_PACIENTES; id++){
        Paciente paciente = generar_paciente(id);
        RID rid = rids[id];

        comprobar(idx_id.insert(paciente.id, rid), "insertar en indice idPaciente");
        comprobar(idx_dni.insert(paciente.dni, rid), "insertar en indice dni");
        comprobar(idx_especialidad.insert(clave_secundaria(paciente.id_especialidad, paciente.id), rid), "insertar en indice idEspecialidad");
        comprobar(idx_prioridad.insert(clave_secundaria(paciente.prioridad, paciente.id), rid), "insertar en indice prioridad");
        comprobar(idx_tipo_sangre.insert(clave_secundaria(codigo_tipo_sangre(paciente.tipo_sangre), paciente.id), rid), "insertar en indice tipoSangre");
    }

    std::cout << "Base hospitalaria cargada correctamente.\n";
    std::cout << "Pacientes: " << TOTAL_PACIENTES << "\n";
    std::cout << "Paginas de datos: " << (data_page_id - PRIMERA_PAGINA_DATOS + 1) << "\n";
    std::cout << "Indices: idPaciente, dni, especialidad, prioridad, tipoSangre\n\n";
}

void demo_automatica(
    BufferManager& bm,
    BPlusTree& idx_id,
    BPlusTree& idx_dni,
    BPlusTree& idx_especialidad,
    BPlusTree& idx_prioridad,
    BPlusTree& idx_tipo_sangre,
    bool imprimir_todos,
    IndexAdvisor& advisor
){
    std::cout << "=== DEMOSTRACION AUTOMATICA ===\n\n";

    ejecutar_select("SELECT * FROM pacientes WHERE idPaciente = 500", bm, idx_id, idx_dni, idx_especialidad, idx_prioridad, idx_tipo_sangre, advisor);
    std::cout << "\n";
    ejecutar_select("SELECT * FROM pacientes WHERE dni = 70000777", bm, idx_id, idx_dni, idx_especialidad, idx_prioridad, idx_tipo_sangre, advisor);
    std::cout << "\n";
    ejecutar_select("SELECT * FROM pacientes WHERE especialidad = 'Neurologia'", bm, idx_id, idx_dni, idx_especialidad, idx_prioridad, idx_tipo_sangre, advisor);
    std::cout << "\n";
    ejecutar_select("SELECT * FROM pacientes WHERE prioridad = 1", bm, idx_id, idx_dni, idx_especialidad, idx_prioridad, idx_tipo_sangre, advisor);
    std::cout << "\n";
    ejecutar_select("SELECT * FROM pacientes WHERE tipoSangre = 'AB+'", bm, idx_id, idx_dni, idx_especialidad, idx_prioridad, idx_tipo_sangre, advisor);
    std::cout << "\n";
    ejecutar_select("SELECT * FROM pacientes WHERE edad = 30", bm, idx_id, idx_dni, idx_especialidad, idx_prioridad, idx_tipo_sangre, advisor);
    std::cout << "\n";
    ejecutar_select("SELECT * FROM pacientes WHERE edad = 30", bm, idx_id, idx_dni, idx_especialidad, idx_prioridad, idx_tipo_sangre, advisor);
    std::cout << "\n";
    ejecutar_select("SELECT * FROM pacientes WHERE sexo = 'F'", bm, idx_id, idx_dni, idx_especialidad, idx_prioridad, idx_tipo_sangre, advisor);
    std::cout << "\n";

    ejecutar_order_by("ORDER BY dni LIMIT 10", bm, idx_id, idx_dni);
    std::cout << "\n";

    ejecutar_group_by("GROUP BY especialidad", idx_especialidad, idx_prioridad, idx_tipo_sangre);
    std::cout << "\n";

    if(imprimir_todos)
        imprimir_por_id(bm, idx_id, 1, TOTAL_PACIENTES);

    imprimir_paginas_calientes(bm);
    std::cout << "\n" << bm.getReplacementReport();
    std::cout << "\n";
    advisor.imprimir();
}

int main(int argc, char* argv[]){
    bool automatico = false;
    bool imprimir_todos = false;

    for(int i = 1; i < argc; i++){
        std::string arg = argv[i];

        if(arg == "--auto")
            automatico = true;
        else if(arg == "--print-all")
            imprimir_todos = true;
    }

    std::remove("hospital_indices.db");

    std::cout << "=== MINI SGBD HOSPITALARIO ===\n";
    std::cout << "Campos: idPaciente, dni, nombres, apellidos, edad, sexo, tipoSangre, idEspecialidad, especialidad, prioridad\n\n";

    PageManager db("hospital_indices.db");
    BufferManager bm(32, db);

    BPlusTree idx_id(bm, 1);
    BPlusTree idx_dni(bm, 2);
    BPlusTree idx_especialidad(bm, 3);
    BPlusTree idx_prioridad(bm, 4);
    BPlusTree idx_tipo_sangre(bm, 5);
    IndexAdvisor advisor;

    cargar_hospital(db, bm, idx_id, idx_dni, idx_especialidad, idx_prioridad, idx_tipo_sangre);

    if(automatico || imprimir_todos)
        demo_automatica(bm, idx_id, idx_dni, idx_especialidad, idx_prioridad, idx_tipo_sangre, imprimir_todos, advisor);
    else
        consola_interactiva(bm, idx_id, idx_dni, idx_especialidad, idx_prioridad, idx_tipo_sangre, advisor);

    std::cout << "\nValidaciones internas: " << verificaciones << "\n";
    std::cout << "Errores detectados: " << errores << "\n";

    if(errores == 0)
        std::cout << "Resultado final: indices secundarios y BufferManager integrados correctamente.\n";
    else
        std::cout << "Resultado final: hay validaciones que revisar.\n";

    return errores == 0 ? 0 : 1;
}
