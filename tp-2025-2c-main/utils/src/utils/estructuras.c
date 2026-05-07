#include <utils/estructuras.h>
#include <utils/serializacion.h>
t_query* crearQuery(char* archivoQuery,int prioridad){
    t_query* query = malloc(sizeof(t_query));
    query->id = -1;
    query->prioridad = prioridad;
    query->pc = 0;
    query->codAccion = CREACION_QUERY;
    query->archivoQueryLength = strlen(archivoQuery)+1;
    query->archivoQuery = string_duplicate(archivoQuery);
    return query;
}

void enviarQuery(t_query* query,int socket,codigo_operacion codOp){
    t_buffer* buffer = serializarQuery(query);
    enviarPaquete(socket,codOp,buffer);
}

void liberarQuery(t_query* query){
	free(query->archivoQuery);
	free(query);
}

t_query_master* crearQueryMaster(t_query* query,int socketQueryControl){
    t_query_master* queryMaster = malloc(sizeof(t_query_master));
    queryMaster->query = query;
    pthread_mutex_init(&queryMaster->mutexPrioridad, NULL);
    pthread_mutex_init(&queryMaster->mutexCodAccion, NULL);
    pthread_mutex_init(&queryMaster->mutexDesconexionQueryControl, NULL);
    queryMaster->socketQueryControl = socketQueryControl;
    queryMaster->desconexionQueryControl = false;
    queryMaster->agingIniciado = false;
    return queryMaster;
}

void liberarQueryMaster(t_query_master* queryMaster){
    liberarQuery(queryMaster->query);
    if (queryMaster->agingIniciado) {
        pthread_join(queryMaster->hiloAging, NULL);
    }
    pthread_mutex_destroy(&queryMaster->mutexPrioridad);
    pthread_mutex_destroy(&queryMaster->mutexCodAccion);
    pthread_mutex_destroy(&queryMaster->mutexDesconexionQueryControl);
    close(queryMaster->socketQueryControl);
    free(queryMaster);
}

t_respuesta_query* crearRespuestaQuery(char* mensaje,int pc,char* nombreFile,char* tag,codigo_accion_query codRespuesta){
    t_respuesta_query* respuestaQuery = malloc(sizeof(t_respuesta_query));
    respuestaQuery->codRespuesta = codRespuesta;
    respuestaQuery->pc = pc;
    respuestaQuery->mensajeLenght = strlen(mensaje)+1;
    respuestaQuery->mensaje = string_duplicate(mensaje);
    respuestaQuery->nombreFileLenght = strlen(nombreFile)+1;
    respuestaQuery->nombreFile = string_duplicate(nombreFile);
    respuestaQuery->tagLenght = strlen(tag)+1;
    respuestaQuery->tag = string_duplicate(tag);
    return respuestaQuery;
}

void enviarRespuestaQuery(t_respuesta_query* respuestaQuery,int socket,codigo_operacion codOp){
    t_buffer* buffer = serializarRespuestaQuery(respuestaQuery);
    enviarPaquete(socket,codOp,buffer);
}

void liberarRespuestaQuery(t_respuesta_query* respuestaQuery){
	free(respuestaQuery->mensaje);
    free(respuestaQuery->nombreFile);
    free(respuestaQuery->tag);
	free(respuestaQuery);
}

t_instruccion* crearInstruccion(int queryID,codigo_instruccion codigoIns,char* fileTag ,char* nombreFile,char* tag,int tamanio,
    int direccionBase,void* contenido,int contenidoLength,char* nombreFileDestino,char* tagDestino, int nroBloque){
    t_instruccion* instruccion = malloc(sizeof(t_instruccion));
    instruccion->queryID = queryID;
    instruccion->codigoIns = codigoIns;
    instruccion->fileTagLength = strlen(fileTag)+1;
    instruccion->fileTag = string_duplicate(fileTag);
    instruccion->nombreFileLength = strlen(nombreFile)+1;
    instruccion->nombreFile = string_duplicate(nombreFile);
    instruccion->tagLength = strlen(tag)+1;
    instruccion->tag = string_duplicate(tag);
    instruccion->tamanio = tamanio;
    instruccion->direccionBase = direccionBase;
    instruccion->contenidoLength = contenidoLength;
    instruccion->contenido = malloc(contenidoLength);
    memcpy(instruccion->contenido, contenido, contenidoLength);
    instruccion->nombreFileDestinoLength = strlen(nombreFileDestino)+1;
    instruccion->nombreFileDestino = string_duplicate(nombreFileDestino);
    instruccion->tagDestinoLength = strlen(tagDestino)+1;
    instruccion->tagDestino = string_duplicate(tagDestino);
    instruccion->nroBloque = nroBloque;
    return instruccion;
}

void enviarInstruccion(t_instruccion* instruccion,int socket){
    t_buffer* buffer = serializarInstruccion(instruccion);
    enviarPaquete(socket,WORKER,buffer);
}

void liberarInstruccion(t_instruccion* instruccion){
	free(instruccion->nombreFile);
    free(instruccion->tag);
    free(instruccion->fileTag);
    free(instruccion->contenido);
    free(instruccion->nombreFileDestino);
    free(instruccion->tagDestino);
	free(instruccion);
}

t_respuesta_storage* crearRespuestaStorage(codigo_respuesta_storage codigo_respuesta,void* contenido,char* error,int sizeBlock){
    t_respuesta_storage* respuestaStorage = malloc(sizeof(t_respuesta_storage));
    respuestaStorage->codRespuesta = codigo_respuesta;
    respuestaStorage->contenidoLength = sizeBlock;
    respuestaStorage->contenido = malloc(respuestaStorage->contenidoLength);
    memcpy(respuestaStorage->contenido, contenido, respuestaStorage->contenidoLength);
    respuestaStorage->errorLength = strlen(error)+1;
    respuestaStorage->error = string_duplicate(error);
    return respuestaStorage;
}

void enviarRespuestaStorage(t_respuesta_storage* respuestaStorage,int socket){
    t_buffer* buffer = serializarRespuestaStorage(respuestaStorage);
    enviarPaquete(socket,RESPUESTA_STORAGE,buffer);
}

void liberarRespuestaStorage(t_respuesta_storage* respuestaStorage){
	free(respuestaStorage->contenido);
    free(respuestaStorage->error);
	free(respuestaStorage);
}

t_worker* crearWorker(int idWorker, int socketWorker){
    t_worker* worker = malloc(sizeof(t_worker));
    worker->id = idWorker;
    worker->idQuery = -1;
    worker->socketWorker = socketWorker; 
    return worker;
}

void liberarWorker(t_worker* worker){
    close(worker->socketWorker);
	free(worker);
}