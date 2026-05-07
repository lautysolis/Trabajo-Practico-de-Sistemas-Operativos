#include <memoria.h>

t_bitarray *crearBitmapWorker()
{
    int bytesNecesarios = (cantMarcos + 7) / 8;

    bitarray = malloc(bytesNecesarios);
    memset(bitarray, 0, bytesNecesarios);
    t_bitarray *bitmap = bitarray_create_with_mode(bitarray, bytesNecesarios, LSB_FIRST);
    
    return bitmap;
}

int liberarMarco(char *fileTagOrigen, int numPagOrigen, int idQuery)
{
    int marcoLibre;
    t_paginaGlobal *victima;

    char *algoritmo = configuracion->algoritmoRemplazo;

    if (strcmp(algoritmo, "LRU") == 0)
    {
        victima = elegirVictimaLRU();
    }
    else if (strcmp(algoritmo, "CLOCK-M") == 0)
    {
        victima = elegirVictimaClockM();
    }
    marcoLibre = victima->pagina->marco;

    char **fileTagTokenizado = string_split(victima->fileTag, ":");
    log_info(logger, "## Query %i:  Se libera el Marco: %i perteneciente al - File: %s - Tag: %s", idQuery, marcoLibre, fileTagTokenizado[0], fileTagTokenizado[1]);

    t_tablaDePaginas *tabla = encontrarTablaConFileTag(victima->fileTag);
    int numPag = encontrarNumeroDePagina(tabla, victima->pagina);
    log_info(logger, "## Query %i:  Se reemplaza la pagina %s / %i por la  %s / %i", idQuery, victima->fileTag, numPag, fileTagOrigen, numPagOrigen);

    liberarVictima(victima, idQuery);

    string_array_destroy(fileTagTokenizado);
    return marcoLibre;
}

t_paginaGlobal *elegirVictimaLRU()
{
    t_paginaGlobal *victima = list_remove(tablaPagGlobal, 0);

    return victima;
}

t_paginaGlobal* elegirVictimaClockM() {
    int vueltas = 1;

    while (1) {
        for (int i = 0; i < list_size(tablaPagGlobal); i++) {
            t_paginaGlobal* posibleVictima = list_get(tablaPagGlobal, puntero_clock);
            t_pagina* pagina = posibleVictima->pagina;
            if (vueltas == 1) {
                if (pagina->uso == 0 && pagina->modificada == 0) {
                    list_remove(tablaPagGlobal, puntero_clock);
                    aumentarPunteroClock();
                    return posibleVictima;
                }
            } else {
                if (pagina->uso == 0 && pagina->modificada == 1) {
                    list_remove(tablaPagGlobal, puntero_clock);
                    aumentarPunteroClock();
                    return posibleVictima;
                } else {
                    pagina->uso = 0;
                }
            }
            aumentarPunteroClock();
        }
        vueltas = (vueltas == 1) ? 2 : 1;
    }
}

void aumentarPunteroClock(){
    puntero_clock++;
    if(puntero_clock==cantMarcos){
        puntero_clock=0;
    }
}

int encontrarNumeroDePagina(t_tablaDePaginas *tabla, t_pagina *pagina)
{
    t_list_iterator *iterator = list_iterator_create(tabla->paginas);
    int indice = -1;

    while (list_iterator_has_next(iterator))
    {
        t_pagina *pag = list_iterator_next(iterator);
        if (pag->marco == pagina->marco)
        {
            indice = list_iterator_index(iterator);
        }
    }
    list_iterator_destroy(iterator);
    return indice;
}

void liberarVictima(t_paginaGlobal *victima, int idQuery)
{
    t_tablaDePaginas *tabla = encontrarTablaConFileTag(victima->fileTag);
    t_pagina *pagina = victima->pagina;

    int numPag = encontrarNumeroDePagina(tabla, pagina);

    pagina->precencia = 0;
    bitarray_clean_bit(bitmap, pagina->marco);

    if (pagina->modificada)
    {
        int marco = pagina->marco;
        
        t_respuesta_storage *resp = realizarEscrituraDeBloque(victima->fileTag, numPag, marco, idQuery);
        liberarRespuestaStorage(resp);
        
        pagina->modificada = 0;
    }

    if (noTienePagsPresentes(tabla))
    {

        destruirTablaDePaginas(tabla);
    }

    destruirPaginaGlobal(victima);
}

bool estaPresente(void *ptr)
{
    t_pagina *pagina = (t_pagina *)ptr;
    return pagina->precencia == 1;
}

bool noTienePagsPresentes(t_tablaDePaginas *tabla)
{
    t_list *paginasPresentes = list_filter(tabla->paginas, estaPresente);
    bool resultado = list_is_empty(paginasPresentes);
    list_destroy(paginasPresentes);
    return resultado;
}

void agregarATablaGlobal(t_pagina *pagina, char *fileTag)
{
    bitarray_set_bit(bitmap, pagina->marco);
    char *algoritmo = configuracion->algoritmoRemplazo;
    t_paginaGlobal *pGlobal = crearPaginaGlobal(pagina, fileTag);

    if (strcmp(algoritmo, "LRU") == 0)
    {
        agregarLRU(pGlobal);
    }
    else if (strcmp(algoritmo, "CLOCK-M") == 0)
    {
        agregarClockM(pGlobal);
    }
}

void agregarClockM(t_paginaGlobal *pGlobal)
{
    list_add_in_index(tablaPagGlobal, pGlobal->pagina->marco, pGlobal);
}

void agregarLRU(t_paginaGlobal *pGlobal)
{
    t_list_iterator *iterator = list_iterator_create(tablaPagGlobal);

    while (list_iterator_has_next(iterator))
    {
        t_paginaGlobal *paginaGlobal = list_iterator_next(iterator);
        if (paginaGlobal->pagina->marco == pGlobal->pagina->marco)
        {
            list_iterator_remove(iterator);
            destruirPaginaGlobal(paginaGlobal);
            break;
        }
    }
    list_iterator_destroy(iterator);

    list_add(tablaPagGlobal, pGlobal);
}

// Crea una tabla de páginas para un determinado fileTag con una única página
t_tablaDePaginas *crearTabla(char *fileTag, int numPag, int flagM, int idQuery)
{
    t_tablaDePaginas *tabla = malloc(sizeof(t_tablaDePaginas));

    tabla->fileTag = string_duplicate(fileTag); // copiar el nombre

    tabla->paginas = list_create();

    int marco = asignarMarco(fileTag, numPag, idQuery);

    char **fileTagTokenizado = string_split(fileTag, ":");
    log_info(logger, "## Query %i:  Se asigna el Marco: %i a la Página: %i perteneciente al - File: %s - Tag: %s", idQuery, marco, numPag, fileTagTokenizado[0], fileTagTokenizado[1]);
    log_info(logger, "## Query %i:  - Memoria Add - File: %s - Tag: %s - Pagina: %i - Marco: %i", idQuery, fileTagTokenizado[0], fileTagTokenizado[1], numPag, marco);

    agregarPagATabla(tabla, numPag, marco, flagM, 1);

    list_add(tablas, tabla);
    string_array_destroy(fileTagTokenizado);
    return tabla;
}

t_paginaGlobal *crearPaginaGlobal(t_pagina *pagina, char *fileTag)
{
    t_paginaGlobal *paginaGlobal = malloc(sizeof(t_paginaGlobal));
    paginaGlobal->pagina = pagina;
    paginaGlobal->fileTag = string_duplicate(fileTag);

    return paginaGlobal;
}

t_pagina *crearPagina(int marco, int flagM, int uso, int presencia)
{
    t_pagina *pagina = malloc(sizeof(t_pagina));
    pagina->marco = marco;
    pagina->precencia = presencia;
    pagina->modificada = flagM;
    pagina->uso = uso;

    return pagina;
}

int asignarMarco(char *fileTag, int numPag, int queryId)
{
    int marcoLibre;

    for (int i = 0; i < cantMarcos; i++)
    {
        if (bitarray_test_bit(bitmap, i) == 0)
        {
            marcoLibre = i;
            return marcoLibre;
        }
    }

    marcoLibre = liberarMarco(fileTag, numPag, queryId);
    return marcoLibre;
}

t_tablaDePaginas *encontrarTablaConFileTag(char *fileTag)
{

    t_list_iterator *iterator = list_iterator_create(tablas);

    while (list_iterator_has_next(iterator))
    {

        t_tablaDePaginas *tablaBuscada = list_iterator_next(iterator);

        if (strcmp(tablaBuscada->fileTag, fileTag) == 0)
        {

            list_iterator_destroy(iterator);

            return tablaBuscada;
        }
    }
    list_iterator_destroy(iterator);

    return NULL;
}

t_respuesta_storage *realizarEscrituraDeBloque(char *fileTag, int numPag, int marco, int idQuery)
{
    char **fileTagTokenizado = string_split(fileTag, ":");
    char *file = fileTagTokenizado[0];
    char *tag = fileTagTokenizado[1];
    int bytesALeer = sizeBlock;
    char *contenido = leerContenidoParaStorage(&bytesALeer, marco, 0);

    t_instruccion *instruccion = crearInstruccion(idQuery, WRITE_BLOCK, fileTag, file, tag, 0, 0, contenido, sizeBlock, "", "", numPag);

    free(contenido);

    enviarInstruccion(instruccion, socketConStorage);
    t_respuesta_storage *respuestaStorage = recibirRespuestaStorage();
    liberarInstruccion(instruccion);
    string_array_destroy(fileTagTokenizado);
    return respuestaStorage;
}

t_respuesta_storage *realizarLecturaDeBloque(char *fileTag, int numPag, int idQuery)
{
    char **fileTagTokenizado = string_split(fileTag, ":");
    char *file = fileTagTokenizado[0];
    char *tag = fileTagTokenizado[1];

    log_info(logger, "## Query %i: - Memoria Miss - File: %s - Tag: %s - Pagina: %i", idQuery, file, tag, numPag);

    t_instruccion *instruccion = crearInstruccion(idQuery, READ_BLOCK, fileTag, file, tag, 0, 0, "", 0, "", "", numPag);

    enviarInstruccion(instruccion, socketConStorage);
    t_respuesta_storage *respuestaStorage = recibirRespuestaStorage();
    
    liberarInstruccion(instruccion);
    string_array_destroy(fileTagTokenizado);
    return respuestaStorage;
}

t_respuesta_storage *escribirMemoria(t_instruccion *instruccion)
{
    int flagM = 1;
    t_tablaDePaginas *tabla;
    int marco;
    t_pagina *pagina;
    t_pagina *paginaInicial;
    int dirFisica;

    char *fileTag = instruccion->fileTag;

    int direccionBase = instruccion->direccionBase;
    int numPag = direccionBase / sizeBlock;
    int desplazamiento = direccionBase % sizeBlock;

    int desplazamientoInicial = desplazamiento;

    void *contenido = malloc(instruccion->contenidoLength);
    void *copiaContenido = contenido;
    memcpy(contenido, instruccion->contenido, instruccion->contenidoLength);

    int bytesDeContenido = instruccion->contenidoLength;
    int marcosNecesarios = (bytesDeContenido + sizeBlock - 1) / sizeBlock;

    t_respuesta_storage *lecturaStoragePorDefault = crearRespuestaStorage(CORRECTO, "Basura", "", 7);
    t_respuesta_storage *lecturaStoragePorError;

    tabla = encontrarTablaConFileTag(fileTag);

    if (!tabla) 
    {
        lecturaStoragePorError = realizarLecturaDeBloque(fileTag, numPag, instruccion->queryID);

        switch (lecturaStoragePorError->codRespuesta)
        {
        case CORRECTO:
            tabla = crearTabla(fileTag, numPag, flagM, instruccion->queryID);
            liberarRespuestaStorage(lecturaStoragePorError);
            break;

        default:
            free(copiaContenido);
            liberarRespuestaStorage(lecturaStoragePorDefault);
            return lecturaStoragePorError;
        }
    }

    for (int i = 0; i < marcosNecesarios; i++)
    {

        if (numPag >= list_size(tabla->paginas))
        {
            lecturaStoragePorError = realizarLecturaDeBloque(fileTag, numPag, instruccion->queryID);

            switch (lecturaStoragePorError->codRespuesta)
            {
            case CORRECTO:
                marco = asignarMarco(fileTag, numPag, instruccion->queryID);

                char **fileTagTokenizado = string_split(fileTag, ":");
                log_info(logger, "## Query %i:  Se asigna el Marco: %i a la Página: %i perteneciente al - File: %s - Tag: %s", instruccion->queryID, marco, numPag, fileTagTokenizado[0], fileTagTokenizado[1]);
                log_info(logger, "## Query %i:  - Memoria Add - File: %s - Tag: %s - Pagina: %i - Marco: %i", instruccion->queryID, fileTagTokenizado[0], fileTagTokenizado[1], numPag, marco);
                
                agregarPagATabla(tabla, numPag, marco, flagM, 1);

                string_array_destroy(fileTagTokenizado);
                liberarRespuestaStorage(lecturaStoragePorError);

                break;
            default:
                free(copiaContenido);
                liberarRespuestaStorage(lecturaStoragePorDefault);
                return lecturaStoragePorError;
            }
        }
        pagina = list_get(tabla->paginas, numPag);
        if (pagina->precencia == 1) 
        {
            pagina->modificada = flagM;
            marco = pagina->marco;
            escribirContenidoEnMarco(&contenido, &bytesDeContenido, marco, desplazamiento);

            desplazamiento = 0;
            numPag++;
        }
        else
        {
            lecturaStoragePorError = realizarLecturaDeBloque(fileTag, numPag, instruccion->queryID);

            switch (lecturaStoragePorError->codRespuesta)
            {
            case CORRECTO:
                pagina->precencia = 1;
                pagina->modificada = flagM;
                pagina->uso = 1;
                pagina->marco = asignarMarco(fileTag, numPag, instruccion->queryID);
                marco = pagina->marco;

                char **fileTagTokenizado = string_split(fileTag, ":");
                log_info(logger, "## Query %i:  Se asigna el Marco: %i a la Página: %i perteneciente al - File: %s - Tag: %s", instruccion->queryID, marco, numPag, fileTagTokenizado[0], fileTagTokenizado[1]);
                log_info(logger, "## Query %i:  - Memoria Add - File: %s - Tag: %s - Pagina: %i - Marco: %i", instruccion->queryID, fileTagTokenizado[0], fileTagTokenizado[1], numPag, marco);

                marco = pagina->marco;
                agregarATablaGlobal(pagina, fileTag);

                escribirContenidoEnMarco(&contenido, &bytesDeContenido, marco, desplazamiento);
                desplazamiento = 0;
                numPag++;
                string_array_destroy(fileTagTokenizado);
                liberarRespuestaStorage(lecturaStoragePorError);
                break;

            default:
                free(copiaContenido);
                liberarRespuestaStorage(lecturaStoragePorDefault);
                return lecturaStoragePorError;
            }
        }
        if (i == 0)
        {
            paginaInicial = pagina;
        }
    }
    dirFisica = (paginaInicial->marco) * sizeBlock + desplazamientoInicial;
    char *contenidoString = castearVoidString(instruccion->contenido, instruccion->contenidoLength);
    log_info(logger, "## Query %i:   Acción: %s - Dirección Física: %i - Valor: %s", instruccion->queryID, codigoAString(instruccion->codigoIns), dirFisica, contenidoString);
    free(copiaContenido);
    free(contenidoString);

    return lecturaStoragePorDefault;
}

t_respuesta_storage *leerMemoria(t_instruccion *instruccion, char **lectura)
{
    int flagM = 0;
    t_tablaDePaginas *tabla;
    int marco;
    t_pagina *pagina;
    char *leido;

    t_pagina *paginaInicial;
    int dirFisica;

    char *fileTag = instruccion->fileTag;

    int direccionBase = instruccion->direccionBase;
    int numPag = direccionBase / sizeBlock;
    int desplazamiento = direccionBase % sizeBlock;

    int desplazamientoInicial = desplazamiento;

    int bytesALeer = instruccion->tamanio;
    int marcosNecesarios = (bytesALeer + sizeBlock - 1) / sizeBlock;

    t_respuesta_storage *lecturaStoragePorDefault = crearRespuestaStorage(CORRECTO, "Basura", "", 7);
    t_respuesta_storage *lecturaStoragePorError;

    tabla = encontrarTablaConFileTag(fileTag);

    if (!tabla) // No existe en nuestra estructura
    {
        lecturaStoragePorError = realizarLecturaDeBloque(fileTag, numPag, instruccion->queryID);
        int bytesLeidos = sizeBlock;

        switch (lecturaStoragePorError->codRespuesta)
        {
        case CORRECTO:
            tabla = crearTabla(fileTag, numPag, flagM, instruccion->queryID);
            pagina = list_get(tabla->paginas, numPag);
            marco = pagina->marco;
            void *contenidoStorage = lecturaStoragePorError->contenido;
            escribirContenidoEnMarco(&(contenidoStorage), &bytesLeidos, marco, 0);
            liberarRespuestaStorage(lecturaStoragePorError);
            break;

        default:
            liberarRespuestaStorage(lecturaStoragePorDefault);
            return lecturaStoragePorError;
        }
    }

    for (int i = 0; i < marcosNecesarios; i++)
    {
        if (numPag >= list_size(tabla->paginas))
        {
            lecturaStoragePorError = realizarLecturaDeBloque(fileTag, numPag, instruccion->queryID);
            int bytesLeidos = sizeBlock;

            switch (lecturaStoragePorError->codRespuesta)
            {
            case CORRECTO:
                marco = asignarMarco(fileTag, numPag, instruccion->queryID);

                char **fileTagTokenizado = string_split(fileTag, ":");
                log_info(logger, "## Query %i:  Se asigna el Marco: %i a la Página: %i perteneciente al - File: %s - Tag: %s", instruccion->queryID, marco, numPag, fileTagTokenizado[0], fileTagTokenizado[1]);
                log_info(logger, "## Query %i:  - Memoria Add - File: %s - Tag: %s - Pagina: %i - Marco: %i", instruccion->queryID, fileTagTokenizado[0], fileTagTokenizado[1], numPag, marco);

                agregarPagATabla(tabla, numPag, marco, flagM, 1);
                void *contenidoStorage = lecturaStoragePorError->contenido;
                escribirContenidoEnMarco(&(contenidoStorage), &bytesLeidos, marco, 0);
                string_array_destroy(fileTagTokenizado);
                if(lecturaStoragePorError!=NULL){
                    liberarRespuestaStorage(lecturaStoragePorError);
                }
                break;

            default:
                liberarRespuestaStorage(lecturaStoragePorDefault);
                return lecturaStoragePorError;
            }
        }
        pagina = list_get(tabla->paginas, numPag);
        if (pagina->precencia == 1) // Esta presente la pag
        {
            pagina->modificada = flagM;
            marco = pagina->marco;
            leido = leerContenidoParaMaster(&bytesALeer, marco, desplazamiento);
            string_append(lectura, leido);
            free(leido);

            desplazamiento = 0;
            numPag++;
        }
        else
        {
            lecturaStoragePorError = realizarLecturaDeBloque(fileTag, numPag, instruccion->queryID);
            int bytesLeidos = sizeBlock;

            switch (lecturaStoragePorError->codRespuesta)
            {
            case CORRECTO:
                pagina->precencia = 1;
                pagina->modificada = flagM;
                pagina->uso = 1;
                pagina->marco = asignarMarco(fileTag, numPag, instruccion->queryID);
                marco = pagina->marco;
                char **fileTagTokenizado = string_split(fileTag, ":");
                log_info(logger, "## Query %i:  Se asigna el Marco: %i a la Página: %i perteneciente al - File: %s - Tag: %s", instruccion->queryID, marco, numPag, fileTagTokenizado[0], fileTagTokenizado[1]);
                log_info(logger, "## Query %i:  - Memoria Add - File: %s - Tag: %s - Pagina: %i - Marco: %i", instruccion->queryID, fileTagTokenizado[0], fileTagTokenizado[1], numPag, marco);

                marco = pagina->marco;
                void *contenidoStorage = lecturaStoragePorError->contenido;
                escribirContenidoEnMarco(&(contenidoStorage), &bytesLeidos, marco, 0);
                agregarATablaGlobal(pagina, fileTag);

                leido = leerContenidoParaMaster(&bytesALeer, marco, desplazamiento);
                string_append(lectura, leido);
                free(leido);
                desplazamiento = 0;
                numPag++;
                string_array_destroy(fileTagTokenizado);
                if(lecturaStoragePorError!=NULL){
                    liberarRespuestaStorage(lecturaStoragePorError);
                }
                break;

            default:
                liberarRespuestaStorage(lecturaStoragePorDefault);
                return lecturaStoragePorError;
            }
        }

        if (i == 0)
        {
            paginaInicial = pagina;
        }
    }
    dirFisica = (paginaInicial->marco) * sizeBlock + desplazamientoInicial;
    log_info(logger, "## Query %i:   Acción: %s - Dirección Física: %i - Valor: %s", instruccion->queryID, codigoAString(instruccion->codigoIns), dirFisica, *lectura);
    return lecturaStoragePorDefault;
}

t_respuesta_storage *persistirMemoriaCompleta(int idQuery)
{
    t_respuesta_storage *respuesta = crearRespuestaStorage(CORRECTO, "Basura", "", 7);
    t_list_iterator *iterator = list_iterator_create(tablas);

    while (list_iterator_has_next(iterator))
    {
        t_tablaDePaginas *tabla = list_iterator_next(iterator);
        liberarRespuestaStorage(respuesta);
        respuesta = persistirMemoria(tabla->fileTag, idQuery);
        if (respuesta->codRespuesta != CORRECTO)
        {
            list_iterator_destroy(iterator);
            return respuesta;
        }
    }
    list_iterator_destroy(iterator);
    return respuesta;
}

t_respuesta_storage *persistirMemoria(char *fileTag, int idQuery)
{
    t_respuesta_storage *respuestaEscritura = crearRespuestaStorage(CORRECTO, "Basura", "", 7);

    t_tablaDePaginas *tablaAPersistir = encontrarTablaConFileTag(fileTag);

    if (!tablaAPersistir) // No existe en nuestra estructura
    {
        return respuestaEscritura;
    }

    t_list_iterator *iterator = list_iterator_create(tablaAPersistir->paginas);

    while (list_iterator_has_next(iterator))
    {
        t_pagina *pagina = list_iterator_next(iterator);
        if (pagina->modificada == 1)
        {
            int numPag = list_iterator_index(iterator);
            int marco = pagina->marco;
            liberarRespuestaStorage(respuestaEscritura);
            respuestaEscritura = realizarEscrituraDeBloque(fileTag, numPag, marco, idQuery);
            pagina->modificada = 0;
            if (respuestaEscritura->codRespuesta != CORRECTO)
            {
                list_iterator_destroy(iterator);
                return respuestaEscritura;
            }
        }
    }
    list_iterator_destroy(iterator);
    return respuestaEscritura;
}

void escribirContenidoEnMarco(void **contenido, int *contenidoLength, int marco, int desplazamiento)
{
    int contenidoAEscribir = sizeBlock - desplazamiento;

    if (*contenidoLength < contenidoAEscribir)
    {
        contenidoAEscribir = *contenidoLength;
    }

    uint8_t *destino = memoriaInterna + (marco * sizeBlock) + desplazamiento;

    memcpy(destino, *contenido, contenidoAEscribir);

    *contenido = (char *)(*contenido) + contenidoAEscribir;
    *contenidoLength -= contenidoAEscribir;
}

char *leerContenidoParaMaster(int *bytesALeer, int marco, int desplazamiento)
{
    int contenidoALeer = sizeBlock - desplazamiento;

    if (*bytesALeer < contenidoALeer)
    {
        contenidoALeer = *bytesALeer;
    }

    char *leido = malloc(contenidoALeer + 1);

    uint8_t *src = memoriaInterna + (marco * sizeBlock) + desplazamiento;

    memcpy(leido, src, contenidoALeer);

    leido[contenidoALeer] = '\0';

    *bytesALeer -= contenidoALeer;

    return leido;
}

char *leerContenidoParaStorage(int *bytesALeer, int marco, int desplazamiento)
{
    int contenidoALeer = sizeBlock - desplazamiento;

    if (*bytesALeer < contenidoALeer)
    {
        contenidoALeer = *bytesALeer;
    }

    char *leido = malloc(contenidoALeer);

    uint8_t *src = memoriaInterna + (marco * sizeBlock) + desplazamiento;

    memcpy(leido, src, contenidoALeer);

    *bytesALeer -= contenidoALeer;

    return leido;
}

void agregarPagATabla(t_tablaDePaginas *tabla, int numPag, int marco, int flagM, int presencia)
{
    t_pagina *pagina = crearPagina(marco, flagM, 1, presencia);

    if (presencia == 1)
    {
        agregarATablaGlobal(pagina, tabla->fileTag);
    }

    if (numPag >= list_size(tabla->paginas))
    {
        agregarPaginasAusentes(tabla, list_size(tabla->paginas), numPag);
    }

    list_add(tabla->paginas, pagina);
}

void agregarPaginasAusentes(t_tablaDePaginas *tabla, int desde, int hasta)
{
    for (int i = desde; i < hasta; i++)
    {
        t_pagina *pagina = crearPagina(0, 0, 1, 0);
        list_add(tabla->paginas, pagina);
    }
}

char *codigoAString(codigo_instruccion codigo)
{
    switch (codigo)
    {
    case CREATE:
        return "CREATE";
    case TRUNCATE:
        return "TRUNCATE";
    case WRITE:
        return "WRITE";
    case READ:
        return "READ";
    case TAG:
        return "TAG";
    case COMMIT:
        return "COMMIT";
    case FLUSH:
        return "FLUSH";
    case DELETE:
        return "DELETE";
    case END:
        return "END";
    default:
        return NULL;
    }
}
