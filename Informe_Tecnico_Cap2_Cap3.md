# Informe Tecnico - Capitulos 2 y 3

## Capitulo 2: Metodos de Acceso

### 2.1 Concepto general

Un metodo de acceso es la forma en que el sistema de base de datos encuentra los registros guardados en disco. Sin un metodo de acceso, para buscar un registro seria necesario revisar pagina por pagina hasta encontrarlo. Esto se conoce como busqueda secuencial y puede ser lenta cuando la base crece.

En este Mini SGBD se usa el arbol B+ como metodo de acceso. El objetivo del arbol B+ es encontrar rapidamente el RID de un registro. El RID indica la ubicacion exacta del registro dentro del archivo de datos:

```text
RID = (page_id, slot_id)
```

Esto significa:

- `page_id`: pagina donde esta guardado el registro.
- `slot_id`: posicion del registro dentro de esa pagina.

### 2.2 Organizacion por paginas

El sistema trabaja con paginas de tamano fijo. Cada pagina mide 4096 bytes y representa la unidad minima de lectura y escritura en disco.

La estructura basica es:

```text
Page
|-- page_id
|-- next_page
|-- buffer
```

El campo `buffer` contiene los datos reales. En paginas de datos, el buffer puede guardar registros usando `SlottedPage`. En paginas de indice, el buffer guarda nodos del arbol B+.

### 2.3 Page Manager

El `PageManager` se encarga de leer y escribir paginas en el archivo `.db`.

Sus operaciones principales son:

- `read_page`: lee una pagina desde disco.
- `write_page`: escribe una pagina en disco.
- `allocate_page`: reserva una nueva pagina al final del archivo.
- `sync`: fuerza la escritura fisica del archivo.

De forma simple:

```text
Archivo .db <-> PageManager <-> Page
```

### 2.4 Buffer Manager

El `BufferManager` evita leer siempre desde disco. Mantiene algunas paginas cargadas en memoria RAM dentro de frames.

Cuando se solicita una pagina:

1. Si ya esta en memoria, se devuelve directamente.
2. Si no esta en memoria, se lee desde disco usando `PageManager`.
3. Si el buffer esta lleno, se escoge una pagina victima usando LRU.

El algoritmo LRU significa "Least Recently Used", es decir, se reemplaza la pagina que fue usada hace mas tiempo.

Tambien se usan dos operaciones importantes:

- `unpinPage`: indica que una pagina ya no esta siendo usada.
- `flushPage`: guarda en disco una pagina modificada.

Estas operaciones permiten integrar correctamente el arbol B+ con el Buffer Manager.

### 2.5 Slotted Page y registros

Las paginas de datos usan `SlottedPage`. Esta estructura permite guardar varios registros dentro de una misma pagina.

Ejemplo:

```text
Pagina 10
|-- Slot 0 -> 1|Ana|20
|-- Slot 1 -> 2|Luis|21
|-- Slot 2 -> 3|Maria|22
```

Si el registro de Luis esta en la pagina 10 y slot 1, su RID es:

```text
RID(10, 1)
```

El arbol B+ guarda claves asociadas a estos RID.

## Capitulo 3: Arbol B+

### 3.1 Objetivo del arbol B+

El arbol B+ permite buscar registros usando una clave, por ejemplo el ID de un alumno.

En vez de revisar todos los registros, se consulta el indice:

```text
clave -> RID
```

Ejemplo:

```text
2 -> RID(10, 1)
```

Esto significa que el alumno con ID 2 esta en la pagina 10, slot 1.

### 3.2 Tipos de nodos

El arbol B+ usa dos tipos de nodos:

#### Nodo interno

Sirve para guiar la busqueda. No guarda registros finales, solo claves separadoras e hijos.

Ejemplo:

```text
[50]
```

Si la clave buscada es menor que 50, se baja al hijo izquierdo. Si es mayor o igual, se baja al hijo derecho.

#### Nodo hoja

Guarda las claves reales junto con sus RID.

Ejemplo:

```text
[1 -> RID(10,0), 2 -> RID(10,1), 3 -> RID(10,2)]
```

En un arbol B+, todas las busquedas terminan en una hoja.

### 3.3 Busqueda

El algoritmo de busqueda funciona asi:

1. Se empieza en la raiz.
2. Si la pagina es un nodo interno, se escoge el hijo correcto.
3. Se baja de pagina en pagina usando el Buffer Manager.
4. Al llegar a una hoja, se busca la clave.
5. Si existe, se devuelve su RID.

Ejemplo:

```text
Buscar clave 2
    |
    v
Arbol B+ devuelve RID(10,1)
    |
    v
SlottedPage lee pagina 10, slot 1
    |
    v
Resultado: 2|Luis|21
```

### 3.4 Insercion de claves

La insercion busca primero la hoja donde debe ir la clave.

Si la hoja tiene espacio:

1. Se copia el contenido de la hoja.
2. Se agrega la nueva clave.
3. Se ordenan las claves.
4. Se reescribe la hoja.
5. Se marca la pagina como modificada y se guarda.

Si la hoja esta llena, se realiza un split.

### 3.5 Division de nodos (Splitting)

El split ocurre cuando un nodo ya no tiene espacio para una nueva clave.

En una hoja:

1. Se copian las claves actuales.
2. Se agrega la nueva clave.
3. Se ordenan todas las claves.
4. Se crea una nueva hoja usando `allocatePage`.
5. La mitad de claves queda en la hoja antigua.
6. La otra mitad pasa a la nueva hoja.
7. La primera clave de la nueva hoja se promociona al padre.

Ejemplo:

```text
Hoja llena:
[1, 2, 3, 4]

Insertar 5:
[1, 2]   [3, 4, 5]
          ^
          clave 3 sube al padre
```

Si la hoja era la raiz, se crea una nueva raiz interna.

### 3.6 Eliminacion de claves

La eliminacion implementada sigue estos pasos:

1. Se busca la hoja donde deberia estar la clave.
2. Si la clave no existe, la operacion falla.
3. Si existe, se elimina de la hoja.
4. La hoja se reordena y se guarda.
5. Si la hoja queda con pocas claves, se intenta fusionar con la hoja derecha.

Esta eliminacion esta implementada de forma simplificada. Es suficiente para demostrar el funcionamiento integrado, pero la fusion recursiva de nodos internos puede considerarse una mejora futura.

### 3.7 Fusion de nodos (Merging)

El merging se usa cuando una hoja queda con pocas claves despues de eliminar.

La version implementada fusiona una hoja con su hoja derecha si:

- Ambas son hojas.
- Tienen el mismo padre.
- La suma de sus claves entra en una sola hoja.

Proceso:

1. Se copian las claves de la hoja derecha a la izquierda.
2. La hoja izquierda apunta a la siguiente hoja de la derecha.
3. El padre elimina la referencia a la hoja derecha.
4. Si el padre era la raiz y queda vacio, la hoja izquierda pasa a ser la nueva raiz.

### 3.8 Integracion con Buffer Manager

Cada nodo B+ se guarda como una pagina normal. Por eso el arbol no maneja archivos directamente; trabaja mediante el Buffer Manager.

Cuando el arbol necesita una pagina:

```text
BPlusTree -> BufferManager -> PageManager -> archivo .db
```

Cuando modifica un nodo:

1. La pagina se marca como sucia con `unpinPage(page_id, true)`.
2. Se escribe a disco con `flushPage(page_id)`.

Esto permite persistencia: despues de cerrar y reabrir el archivo, el arbol puede seguir buscando si se conserva el `root_page_id`.

### 3.9 Persistencia

La persistencia significa que los datos quedan guardados en el archivo `.db`.

En el B+ Tree hay un detalle importante: cuando ocurre un split de raiz, la raiz puede cambiar de pagina. Por eso el sistema expone:

```text
get_root_page_id()
```

Ese valor debe guardarse como metadato del indice. En esta implementacion de prueba, se guarda en una variable y se usa al reabrir la base.

### 3.10 Pruebas realizadas

Se agrego una prueba completa del B+ Tree que valida:

- Busqueda en raiz hoja.
- Busqueda bajando por nodo interno.
- Integracion con datos reales de alumnos usando `SlottedPage`.
- Insercion de suficientes claves para forzar split.
- Persistencia despues de reabrir el archivo.
- Eliminacion de claves.
- Fusion de hojas.
- Persistencia despues de eliminar y fusionar.
- Demostracion con 1000 registros de alumnos.

El resultado obtenido fue:

```text
Pruebas correctas: 34
Pruebas fallidas: 0
RESULTADO B+ TREE: OK
```

En la demostracion de 1000 registros se insertan alumnos con formato:

```text
id|Nombre Apellido|edad
```

Por ejemplo:

```text
500|Alejandro Vargas|18
```

La prueba realiza:

1. Insercion de 1000 registros en paginas de datos usando `SlottedPage`.
2. Construccion del indice B+ usando `id -> RID`.
3. Busqueda de claves representativas: 1, 500 y 1000.
4. Lectura del registro real usando el RID devuelto por el indice.
5. Eliminacion de claves 10, 500 y 999.
6. Verificacion de que la clave 500 ya no existe.
7. Verificacion de que la clave 501 sigue existiendo.
8. Reapertura del archivo `.db` para comprobar persistencia.

Adicionalmente, la demostracion puede ejecutarse con `--print-all` para imprimir todos los registros antes de eliminar y luego volver a imprimirlos despues de eliminar. En esa segunda impresion, las claves eliminadas aparecen como no encontradas en el indice.

Salida representativa:

```text
=== DEMOSTRACION B+ TREE INTEGRADO ===

Archivo de prueba: test_bplustree_1000.db
Cantidad de registros: 1000

1. Insertando registros en paginas de datos...
   Registros guardados en paginas 100 a 106.
   Paginas de datos usadas: 7

2. Construyendo indice B+ sobre la clave id...
   El indice guarda entradas con forma: id -> RID(page_id, slot_id).
   Root page id del indice B+: 108

3. Buscando registros mediante el indice...
   id=1 -> RID(100, 0) -> 1|Ana Quispe|19
   id=500 -> RID(103, 29) -> 500|Alejandro Vargas|18
   id=1000 -> RID(106, 63) -> 1000|Alejandro Castillo|18

4. Eliminando claves del indice...
   Se eliminaron del indice los ids 10, 500 y 999.
   Busqueda posterior de id=500: no encontrado.
   Busqueda posterior de id=501: encontrado en RID(103, 30).

5. Reabriendo el archivo para comprobar persistencia...
   id=1000 despues de reabrir -> RID(106, 63) -> 1000|Alejandro Castillo|18
   id=500 despues de reabrir: no encontrado.

Resultado final: el B+ Tree funciona integrado con BufferManager y PageManager.
```

### 3.11 Limitaciones y mejoras futuras

La eliminacion y merging estan implementados de forma simplificada. La fusion de hojas funciona y se demuestra con persistencia, pero una implementacion completa deberia agregar:

- Redistribucion de claves entre hermanos.
- Fusion recursiva de nodos internos.
- Liberacion formal de paginas eliminadas.
- Guardado permanente del `root_page_id` en una pagina de metadatos.

Estas mejoras permitirian que el arbol B+ soporte casos mas complejos de eliminacion.
