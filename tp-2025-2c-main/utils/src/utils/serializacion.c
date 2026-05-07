#include <utils/serializacion.h>

t_buffer* serializarQuery(t_query* query){
    int32_t size = sizeof(int32_t) * 5 + query->archivoQueryLength; 
    t_buffer* buffer = crearBuffer(size);
    agregarBuffer(buffer,&(query->id),sizeof(int32_t));
    agregarBuffer(buffer,&(query->prioridad),sizeof(int32_t));
    agregarBuffer(buffer,&(query->pc),sizeof(int32_t));
    agregarBuffer(buffer,&(query->codAccion),sizeof(codigo_accion_query));
    agregarBuffer(buffer,&(query->archivoQueryLength),sizeof(int32_t));
    agregarBuffer(buffer,query->archivoQuery,query->archivoQueryLength);
    return buffer;
}

t_query* deserializarQuery(t_buffer* buffer) {
    buffer->offset = 0;
    t_query* query = malloc(sizeof(t_query));
    leerBuffer(buffer,&(query->id),sizeof(int32_t));
    leerBuffer(buffer,&(query->prioridad),sizeof(int32_t));
    leerBuffer(buffer,&(query->pc),sizeof(int32_t));
    leerBuffer(buffer,&(query->codAccion),sizeof(codigo_accion_query));
    leerBuffer(buffer,&(query->archivoQueryLength),sizeof(int32_t));
    query->archivoQuery = malloc(query->archivoQueryLength);
    leerBuffer(buffer,query->archivoQuery,query->archivoQueryLength);

    return query;
}

t_buffer* serializarSizeBlock(int32_t sizeBlock){
    int32_t size = sizeof(int32_t);
    t_buffer* buffer = crearBuffer(size);
    agregarBuffer(buffer,&sizeBlock,sizeof(int32_t));
    return buffer;
}

int32_t deserializarSizeBlock(t_buffer* buffer){
    buffer->offset = 0;
    int32_t sizeBlock;
    leerBuffer(buffer,&sizeBlock,sizeof(int32_t));
    return sizeBlock;
}

t_buffer* serializarIdWorker(int32_t idWorker){
    int32_t size = sizeof(int32_t);
    t_buffer* buffer = crearBuffer(size);
    agregarBuffer(buffer,&idWorker,sizeof(int32_t));
    return buffer;
}

int32_t deserializarIdWorker(t_buffer* buffer){
    buffer->offset = 0;
    int32_t idWorker;
    leerBuffer(buffer,&idWorker,sizeof(int32_t));
    return idWorker;
}

t_buffer* serializarInstruccion(t_instruccion* instruccion){
    int32_t size = sizeof(int32_t)*10+sizeof(codigo_instruccion)+instruccion->nombreFileLength+instruccion->tagLength+
    instruccion->contenidoLength+instruccion->nombreFileDestinoLength+instruccion->tagDestinoLength + instruccion->fileTagLength;
    t_buffer* buffer = crearBuffer(size);
    agregarBuffer(buffer,&(instruccion->queryID),sizeof(int32_t));
    agregarBuffer(buffer,&(instruccion->codigoIns),sizeof(codigo_instruccion));

    agregarBuffer(buffer,&(instruccion->fileTagLength),sizeof(int32_t));
    agregarBuffer(buffer,instruccion->fileTag,instruccion->fileTagLength);

    agregarBuffer(buffer,&(instruccion->nombreFileLength),sizeof(int32_t));
    agregarBuffer(buffer,instruccion->nombreFile,instruccion->nombreFileLength);
    agregarBuffer(buffer,&(instruccion->tagLength),sizeof(int32_t));
    agregarBuffer(buffer,instruccion->tag,instruccion->tagLength);
    agregarBuffer(buffer,&(instruccion->tamanio),sizeof(int32_t));
    agregarBuffer(buffer,&(instruccion->direccionBase),sizeof(int32_t));
    agregarBuffer(buffer,&(instruccion->contenidoLength),sizeof(int32_t));
    agregarBuffer(buffer,instruccion->contenido,instruccion->contenidoLength);
    agregarBuffer(buffer,&(instruccion->nombreFileDestinoLength),sizeof(int32_t));
    agregarBuffer(buffer,instruccion->nombreFileDestino,instruccion->nombreFileDestinoLength);
    agregarBuffer(buffer,&(instruccion->tagDestinoLength),sizeof(int32_t));
    agregarBuffer(buffer,instruccion->tagDestino,instruccion->tagDestinoLength);
    agregarBuffer(buffer,&(instruccion->nroBloque),sizeof(int32_t));
    return buffer;
}

t_instruccion* deserializarInstruccion(t_buffer* buffer){
    buffer->offset = 0;
    t_instruccion* instruccion = malloc(sizeof(t_instruccion));
    leerBuffer(buffer,&(instruccion->queryID),sizeof(int32_t));
    leerBuffer(buffer,&(instruccion->codigoIns),sizeof(codigo_instruccion));

    leerBuffer(buffer,&(instruccion->fileTagLength),sizeof(int32_t));
    instruccion->fileTag = malloc(instruccion->fileTagLength);
    leerBuffer(buffer,instruccion->fileTag,instruccion->fileTagLength);

    leerBuffer(buffer,&(instruccion->nombreFileLength),sizeof(int32_t));
    instruccion->nombreFile = malloc(instruccion->nombreFileLength);
    leerBuffer(buffer,instruccion->nombreFile,instruccion->nombreFileLength);
    leerBuffer(buffer,&(instruccion->tagLength),sizeof(int32_t));
    instruccion->tag = malloc(instruccion->tagLength);
    leerBuffer(buffer,instruccion->tag,instruccion->tagLength);
    leerBuffer(buffer,&(instruccion->tamanio),sizeof(int32_t));
    leerBuffer(buffer,&(instruccion->direccionBase),sizeof(int32_t));
    leerBuffer(buffer,&(instruccion->contenidoLength),sizeof(int32_t));
    instruccion->contenido = malloc(instruccion->contenidoLength);
    leerBuffer(buffer,instruccion->contenido,instruccion->contenidoLength);
    leerBuffer(buffer,&(instruccion->nombreFileDestinoLength),sizeof(int32_t));
    instruccion->nombreFileDestino = malloc(instruccion->nombreFileDestinoLength);
    leerBuffer(buffer,instruccion->nombreFileDestino,instruccion->nombreFileDestinoLength);
    leerBuffer(buffer,&(instruccion->tagDestinoLength),sizeof(int32_t));
    instruccion->tagDestino = malloc(instruccion->tagDestinoLength);
    leerBuffer(buffer,instruccion->tagDestino,instruccion->tagDestinoLength);
    leerBuffer(buffer,&(instruccion->nroBloque),sizeof(int32_t));
    return instruccion;
}

t_buffer* serializarRespuestaQuery(t_respuesta_query* respuestaQuery){
    int32_t size = sizeof(int32_t)*4 + sizeof(codigo_accion_query) + respuestaQuery->mensajeLenght + 
    respuestaQuery->nombreFileLenght + respuestaQuery->tagLenght; 
    t_buffer* buffer = crearBuffer(size);
    agregarBuffer(buffer,&(respuestaQuery->codRespuesta),sizeof(codigo_accion_query));
    agregarBuffer(buffer,&(respuestaQuery->pc),sizeof(int32_t));
    agregarBuffer(buffer,&(respuestaQuery->mensajeLenght),sizeof(int32_t));
    agregarBuffer(buffer,respuestaQuery->mensaje,respuestaQuery->mensajeLenght);
    agregarBuffer(buffer,&(respuestaQuery->nombreFileLenght),sizeof(int32_t));
    agregarBuffer(buffer,respuestaQuery->nombreFile,respuestaQuery->nombreFileLenght);
    agregarBuffer(buffer,&(respuestaQuery->tagLenght),sizeof(int32_t));
    agregarBuffer(buffer,respuestaQuery->tag,respuestaQuery->tagLenght);
    return buffer;
}

t_respuesta_query* deserializarRespuestaQuery(t_buffer* buffer) {
    buffer->offset = 0;
    t_respuesta_query* respuestaQuery = malloc(sizeof(t_respuesta_query));
    leerBuffer(buffer,&(respuestaQuery->codRespuesta),sizeof(codigo_accion_query));
    leerBuffer(buffer,&(respuestaQuery->pc),sizeof(int32_t));
    leerBuffer(buffer,&(respuestaQuery->mensajeLenght),sizeof(int32_t));
    respuestaQuery->mensaje = malloc(respuestaQuery->mensajeLenght);
    leerBuffer(buffer,respuestaQuery->mensaje,respuestaQuery->mensajeLenght);
    leerBuffer(buffer,&(respuestaQuery->nombreFileLenght),sizeof(int32_t));
    respuestaQuery->nombreFile = malloc(respuestaQuery->nombreFileLenght);
    leerBuffer(buffer,respuestaQuery->nombreFile,respuestaQuery->nombreFileLenght);
    leerBuffer(buffer,&(respuestaQuery->tagLenght),sizeof(int32_t));
    respuestaQuery->tag = malloc(respuestaQuery->tagLenght);
    leerBuffer(buffer,respuestaQuery->tag,respuestaQuery->tagLenght);
    return respuestaQuery;
}

t_buffer* serializarRespuestaStorage(t_respuesta_storage* respuestaStorage){
    int32_t size = sizeof(codigo_respuesta_storage) + sizeof(int32_t) + respuestaStorage->contenidoLength + respuestaStorage->errorLength + sizeof(int32_t);

    t_buffer* buffer = crearBuffer(size);

    agregarBuffer(buffer,&(respuestaStorage->codRespuesta),sizeof(codigo_respuesta_storage));
    agregarBuffer(buffer, &(respuestaStorage->contenidoLength), sizeof(int32_t));
    agregarBuffer(buffer,respuestaStorage->contenido,respuestaStorage->contenidoLength);
    agregarBuffer(buffer,&(respuestaStorage->errorLength),sizeof(int32_t));
    agregarBuffer(buffer,respuestaStorage->error,respuestaStorage->errorLength);

    return buffer;
}

t_respuesta_storage* deserializarRespuestaStorage(t_buffer* buffer){
    buffer->offset = 0;
    t_respuesta_storage* respuestaStorage = malloc(sizeof(t_respuesta_storage));
    leerBuffer(buffer,&(respuestaStorage->codRespuesta),sizeof(codigo_respuesta_storage));
    leerBuffer(buffer,&(respuestaStorage->contenidoLength),sizeof(int32_t));
    respuestaStorage->contenido = malloc(respuestaStorage->contenidoLength);
    leerBuffer(buffer,respuestaStorage->contenido,respuestaStorage->contenidoLength);
    leerBuffer(buffer,&(respuestaStorage->errorLength),sizeof(int32_t));
    respuestaStorage->error = malloc(respuestaStorage->errorLength);
    leerBuffer(buffer,respuestaStorage->error,respuestaStorage->errorLength);
    return respuestaStorage;
}




