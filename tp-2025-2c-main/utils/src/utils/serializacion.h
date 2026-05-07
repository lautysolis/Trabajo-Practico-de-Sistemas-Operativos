#ifndef UTILS_SERIALIZACION_H_
#define UTILS_SERIALIZACION_H_

#include <utils/paquetes.h>
#include <utils/cliente.h>
#include <utils/utils.h>
#include <utils/estructuras.h>

t_buffer* serializarQuery(t_query*);
t_query* deserializarQuery(t_buffer* );
t_buffer* serializarSizeBlock(int32_t);
int32_t deserializarSizeBlock(t_buffer*);
t_buffer* serializarIdWorker(int32_t);
int32_t deserializarIdWorker(t_buffer*);
t_buffer* serializarRespuestaQuery(t_respuesta_query*);
t_respuesta_query* deserializarRespuestaQuery(t_buffer*);
t_buffer* serializarInstruccion(t_instruccion*);
t_instruccion* deserializarInstruccion(t_buffer* );
t_buffer* serializarRespuestaStorage(t_respuesta_storage*);
t_respuesta_storage* deserializarRespuestaStorage(t_buffer*);
#endif