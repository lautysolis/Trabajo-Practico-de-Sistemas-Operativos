#include <ejecucion.h>
#include <memoria.h>

void escucharMaster()
{
    while (!terminar)
    {
        t_paquete *paquete = recibirPaquete(socketConMaster);
        if (paquete == NULL)
        {
            break;
        }
        t_query *queryRecibida = deserializarQuery(paquete->buffer);
        codigo_accion_query codigoAccion = queryRecibida->codAccion;
        int idQuery = queryRecibida->id;
        char *archivoQuery = string_duplicate(queryRecibida->archivoQuery);

        switch (codigoAccion)
        {
        case EJECUCION_QUERY:
            log_info(logger, "## Query %i: Se recibe la Query. El path de operaciones es: %s", idQuery, archivoQuery);
            pthread_mutex_lock(&mutex_query);
            queryActual = queryRecibida;
            pthread_mutex_unlock(&mutex_query);
            sem_post(&sem_query);
            break;
        case DESALOJO_QUERY:
            pthread_mutex_lock(&mutex_interr);
            interrupcion = 1;
            pthread_mutex_unlock(&mutex_interr);
            liberarQuery(queryRecibida);
            break;
        default:
            log_warning(logger, "Codigo de operacion desconocido recibido: %d", paquete->codigoOperacion);
            liberarQuery(queryRecibida);
            break;
        }
        free(archivoQuery);
        eliminarPaquete(paquete);
    }
}

int huboInterrupcion()
{
    pthread_mutex_lock(&mutex_interr);
    if (interrupcion == 1)
    {
        interrupcion = 0;
        pthread_mutex_unlock(&mutex_interr);
        return 1;
    }
    pthread_mutex_unlock(&mutex_interr);
    return 0;
}

void *queryInterpreter()
{
    while (!terminar)
    {
        sem_wait(&sem_query);
        if (terminar)
        {
            pthread_mutex_lock(&mutex_query);
            if (queryActual != NULL)
            {
                liberarQuery(queryActual);
                queryActual = NULL;
            }
            pthread_mutex_unlock(&mutex_query);
            break;
        }
        pthread_mutex_lock(&mutex_query);
        int32_t programCounter = queryActual->pc;
        int32_t queryIDactual = queryActual->id;
        char *archivoQuery = string_duplicate(queryActual->archivoQuery);
        pthread_mutex_unlock(&mutex_query);

        char *pathFinal = crearPathFinal(archivoQuery);

        FILE *f = fopen(pathFinal, "r");

        if (!f)
        {
            log_error(logger, "No se pudo abrir el archivo");
            exit(1);
        }

        char *linea = leerLineaPorIndice(f, programCounter);

        while (linea)
        {
            t_instruccion *instruccionLeida = interpretarInstruccion(linea, queryIDactual);
            log_info(logger, "## Query %i: FETCH - Program Counter: %i - %s", queryIDactual, programCounter, codigoAString(instruccionLeida->codigoIns));
            t_respuesta_storage *respuesta = ejecutarInstruccion(instruccionLeida, programCounter);
            if (respuesta->codRespuesta != CORRECTO)
            {
                procesarError(respuesta, instruccionLeida->nombreFile, instruccionLeida->tag);
                liberarRespuestaStorage(respuesta);
                liberarInstruccion(instruccionLeida);
                free(linea);
                break;
            }

            log_info(logger, "## Query %i: - Instrucción realizada: %s", queryIDactual, codigoAString(instruccionLeida->codigoIns));

            if (huboInterrupcion())
            {
                liberarRespuestaStorage(respuesta);
                respuesta = persistirMemoriaCompleta(queryIDactual);
                programCounter++;

                if (respuesta->codRespuesta != CORRECTO)
                {
                    procesarError(respuesta, instruccionLeida->nombreFile, instruccionLeida->tag);
                }
                else
                {
                    confirmarDesalojo(programCounter, instruccionLeida->nombreFile, instruccionLeida->tag);

                    log_info(logger, "## Query %i: Desalojada por pedido del Master", queryIDactual);
                }
                liberarRespuestaStorage(respuesta);
                liberarInstruccion(instruccionLeida);
                free(linea);
                break;
            }

            liberarInstruccion(instruccionLeida);
            liberarRespuestaStorage(respuesta);
            programCounter++;
            free(linea);
            linea = leerLineaPorIndice(f, programCounter);
            if (terminar)
            {
                pthread_mutex_lock(&mutex_query);
                if (queryActual != NULL)
                {
                    liberarQuery(queryActual);
                    queryActual = NULL;
                }
                pthread_mutex_unlock(&mutex_query);
                free(linea);
                break;
            }
        }
        fclose(f);
        free(pathFinal);
        free(archivoQuery);
    }
    return NULL;
}

t_respuesta_storage *ejecutarInstruccion(t_instruccion *instruccion, int32_t programCounter)
{
    t_respuesta_query *respuestaQuery;
    t_respuesta_storage *respuestaStorage;
    int microSegundos = configuracion->retardoMemoria * 1000;

    switch (instruccion->codigoIns)
    {
    case CREATE:
        enviarInstruccion(instruccion, socketConStorage);
        respuestaStorage = recibirRespuestaStorage();
        return respuestaStorage;

    case TRUNCATE:
        enviarInstruccion(instruccion, socketConStorage);
        return respuestaStorage = recibirRespuestaStorage();

    case WRITE:

        // RETARDO DADO POR CONFIG
        usleep(microSegundos);
        respuestaStorage = escribirMemoria(instruccion);

        return respuestaStorage;

    case READ:

        // RETARDO DADO POR CONFIG
        usleep(microSegundos);

        char *lectura = string_new();
        respuestaStorage = leerMemoria(instruccion, &lectura);

        if (respuestaStorage->codRespuesta == CORRECTO)
        {
            respuestaQuery = crearRespuestaQuery(lectura, programCounter + 1, instruccion->nombreFile, instruccion->tag, RESPUESTA_QUERY);
            enviarRespuestaQuery(respuestaQuery, socketConMaster, WORKER);
            liberarRespuestaQuery(respuestaQuery);
        }
        free(lectura);
        return respuestaStorage;

    case TAG:
        enviarInstruccion(instruccion, socketConStorage);
        return respuestaStorage = recibirRespuestaStorage();

    case COMMIT:
        respuestaStorage = persistirMemoria(instruccion->fileTag, instruccion->queryID);

        if (respuestaStorage->codRespuesta == CORRECTO)
        {
            liberarRespuestaStorage(respuestaStorage);
            enviarInstruccion(instruccion, socketConStorage);
            respuestaStorage = recibirRespuestaStorage();
        }

        return respuestaStorage;

    case FLUSH:
        return respuestaStorage = persistirMemoria(instruccion->fileTag, instruccion->queryID);

    case DELETE:
        enviarInstruccion(instruccion, socketConStorage);
        return respuestaStorage = recibirRespuestaStorage();

    case END:
        respuestaQuery = crearRespuestaQuery("Fin de Ejecucion - Exitoso", programCounter + 1, instruccion->nombreFile, instruccion->tag, FIN_QUERY);
        pthread_mutex_lock(&mutex_query);
        liberarQuery(queryActual);
        queryActual = NULL;
        pthread_mutex_unlock(&mutex_query);
        enviarRespuestaQuery(respuestaQuery, socketConMaster, WORKER);
        liberarRespuestaQuery(respuestaQuery);

        respuestaStorage = crearRespuestaStorage(CORRECTO, "Basura", "", 7);
        return respuestaStorage;

    default:
        log_error(logger, "Error al ejecutar instruccion");
        exit(1);
    }
}

codigo_instruccion obtenerCodigoOperacion(char *instruccion)
{
    if (strcmp(instruccion, "CREATE") == 0)
        return CREATE;
    else if (strcmp(instruccion, "TRUNCATE") == 0)
        return TRUNCATE;
    else if (strcmp(instruccion, "WRITE") == 0)
        return WRITE;
    else if (strcmp(instruccion, "READ") == 0)
        return READ;
    else if (strcmp(instruccion, "TAG") == 0)
        return TAG;
    else if (strcmp(instruccion, "COMMIT") == 0)
        return COMMIT;
    else if (strcmp(instruccion, "FLUSH") == 0)
        return FLUSH;
    else if (strcmp(instruccion, "DELETE") == 0)
        return DELETE;
    else if (strcmp(instruccion, "END") == 0)
        return END;
    else
        return -1;
}

t_instruccion *interpretarInstruccion(char *linea, int queryId)
{
    t_instruccion *instruccion;

    char **instruccionPorPartes = string_split(linea, " ");

    codigo_instruccion codigoInstruccion = obtenerCodigoOperacion(instruccionPorPartes[0]);

    char **parametrosTokenizados = NULL;
    char **parametrosTokenizadosParaTag = NULL;

    if (codigoInstruccion != END)
    {
        parametrosTokenizados = string_split(instruccionPorPartes[1], ":");
    }

    switch (codigoInstruccion)
    {
    case CREATE:
        // instruccion(query_ID, codigoIns, fileTag , nombreFile, tag, tamanio, direccionBase, contenido, contenidoLength, nombreFileDestino, tagDestino, nroBloque)
        instruccion = crearInstruccion(queryId, CREATE, instruccionPorPartes[1], parametrosTokenizados[0], parametrosTokenizados[1], 0, 0, "", 1, "", "", 0);
        break;

    case TRUNCATE:
        // instruccion(query_ID, codigoIns, fileTag , nombreFile, tag, tamanio, direccionBase, contenido, nombreFileDestino, tagDestino, nroBloque)
        instruccion = crearInstruccion(queryId, TRUNCATE, instruccionPorPartes[1], parametrosTokenizados[0], parametrosTokenizados[1], atoi(instruccionPorPartes[2]), 0, "", 1, "", "", 0);

        break;

    case WRITE:
        // instruccion(query_ID, codigoIns, fileTag , nombreFile, tag, tamanio, direccionBase, contenido, nombreFileDestino, tagDestino, nroBloque)
        char *str = instruccionPorPartes[3];
        size_t contenidoLength = strlen(str);
        void *contenido = malloc(contenidoLength);
        memcpy(contenido, str, contenidoLength);

        instruccion = crearInstruccion(queryId, WRITE, instruccionPorPartes[1], parametrosTokenizados[0], parametrosTokenizados[1], 0, atoi(instruccionPorPartes[2]), contenido, contenidoLength, "", "", 0);
        free(contenido);
        break;

    case READ:
        // instruccion(query_ID, codigoIns, fileTag , nombreFile, tag, tamanio, direccionBase, contenido, nombreFileDestino, tagDestino, nroBloque)
        instruccion = crearInstruccion(queryId, READ, instruccionPorPartes[1], parametrosTokenizados[0], parametrosTokenizados[1], atoi(instruccionPorPartes[3]), atoi(instruccionPorPartes[2]), "", 1, "", "", 0);
        break;

    case TAG:
        parametrosTokenizadosParaTag = string_split(instruccionPorPartes[2], ":");
        // instruccion(query_ID, codigoIns, fileTag , nombreFile, tag, tamanio, direccionBase, contenido, nombreFileDestino, tagDestino, nroBloque)
        instruccion = crearInstruccion(queryId, TAG, instruccionPorPartes[1], parametrosTokenizados[0], parametrosTokenizados[1], 0, 0, "", 1, parametrosTokenizadosParaTag[0], parametrosTokenizadosParaTag[1], 0);
        string_array_destroy(parametrosTokenizadosParaTag);
        break;

    case COMMIT:
        // instruccion(query_ID, codigoIns, fileTag , nombreFile, tag, tamanio, direccionBase, contenido, nombreFileDestino, tagDestino, nroBloque)
        instruccion = crearInstruccion(queryId, COMMIT, instruccionPorPartes[1], parametrosTokenizados[0], parametrosTokenizados[1], 0, 0, "", 1, "", "", 0);
        break;

    case FLUSH:
        // instruccion(query_ID, codigoIns, fileTag , nombreFile, tag, tamanio, direccionBase, contenido, nombreFileDestino, tagDestino, nroBloque)
        instruccion = crearInstruccion(queryId, FLUSH, instruccionPorPartes[1], parametrosTokenizados[0], parametrosTokenizados[1], 0, 0, "", 1, "", "", 0);
        break;

    case DELETE:
        // instruccion(query_ID, codigoIns, fileTag , nombreFile, tag, tamanio, direccionBase, contenido, nombreFileDestino, tagDestino, nroBloque)
        instruccion = crearInstruccion(queryId, DELETE, instruccionPorPartes[1], parametrosTokenizados[0], parametrosTokenizados[1], 0, 0, "", 1, "", "", 0);
        break;

    case END:
        // instruccion(query_ID, codigoIns, fileTag , nombreFile, tag, tamanio, direccionBase, contenido, nombreFileDestino, tagDestino, nroBloque)
        instruccion = crearInstruccion(queryId, END, "", "", "", 0, 0, "", 1, "", "", 0);
        break;

    default:
        log_error(logger, "Instruccion no reconocida: %s", linea);
        exit(1);
    }

    if (codigoInstruccion != END)
    {
        string_array_destroy(parametrosTokenizados);
    }
    string_array_destroy(instruccionPorPartes);
    return instruccion;
}

char *leerLineaPorIndice(FILE *archivo, int lineaALeer)
{
    rewind(archivo);

    char buffer[256]; // tamaño máximo de línea
    char *resultado = NULL;

    for (int i = 0; fgets(buffer, sizeof(buffer), archivo); i++)
    {
        if (i == lineaALeer)
        {
            // Eliminar el salto de línea si existe
            buffer[strcspn(buffer, "\n")] = '\0';

            // Reservar memoria para copiar la línea
            resultado = malloc(strlen(buffer) + 1);
            if (resultado)
                strcpy(resultado, buffer);
            break;
        }
    }
    return resultado; // NULL si no existía esa línea
}

char *crearPathFinal(char *archivoQuery)
{
    //  /home/utnso/queries/query1.txt
    char *copiaPathQuerys = string_duplicate(configuracion->pathQuerys);
    string_append(&copiaPathQuerys, archivoQuery);

    return copiaPathQuerys;
}