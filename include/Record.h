#ifndef RECORD_H
#define RECORD_H

#include <string>
#include <vector>
#include <sstream>

// Record NO es un tipo de dato nuevo en disco: sigue siendo el mismo
// std::string que ya guardabas via SlottedPage/Table. Lo unico que hace
// es (de)serializar una fila con varios campos hacia/desde ese string,
// usando un delimitador. Es el paso minimo necesario para que Select y
// Project puedan razonar sobre columnas individuales en vez de un blob.
//
// LIMITACION CONOCIDA: si algun campo de texto contiene el delimitador
// (por defecto '|'), la fila se corromperia al separarla. Para un
// proyecto academico esto es aceptable; una version robusta usaria
// escape de caracteres o longitudes de campo prefijadas.
class Record {
public:
    static std::string serialize(const std::vector<std::string>& fields, char delim = '|'){
        std::string out;
        for(size_t i = 0; i < fields.size(); i++){
            out += fields[i];
            if(i + 1 < fields.size())
                out += delim;
        }
        return out;
    }

    static std::vector<std::string> deserialize(const std::string& raw, char delim = '|'){
        std::vector<std::string> fields;
        std::stringstream ss(raw);
        std::string field;

        while(std::getline(ss, field, delim)){
            fields.push_back(field);
        }

        return fields;
    }

    // Ayudas para leer un campo ya tipado, cuando sepas que columna es numerica.
    // Uso tipico dentro de un predicado de Select: Record::as_int(fields[2]) > 18
    static int as_int(const std::string& field){
        return std::stoi(field);
    }

    static double as_double(const std::string& field){
        return std::stod(field);
    }
};

#endif