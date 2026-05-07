#ifndef UTILS_PAQUETES_H_
#define UTILS_PAQUETES_H_

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <assert.h>
#include <commons/string.h>
#include <commons/config.h>
#include <readline/readline.h>
#include <pthread.h>
#include <semaphore.h>

extern t_log* logger;

typedef enum
{
	QUERY,
    MASTER,
	WORKER,
    TAM_BLOQUE,
    HANDSHAKE_WORKER_STORAGE,
    MOCK_RESPUESTA,
    RESPUESTA_STORAGE
} codigo_operacion;

typedef struct {
    int32_t size;
    int32_t offset;
    void* stream; 
} t_buffer;

typedef struct
{
	int codigoOperacion;
	t_buffer* buffer;
} t_paquete;

t_buffer *crearBuffer(int32_t);
void agregarBuffer(t_buffer*, void*, int32_t);
void leerBuffer(t_buffer*, void*, int32_t);
void enviarPaquete(int,codigo_operacion,t_buffer*);
t_paquete* crearPaquete(codigo_operacion,t_buffer*);
void eliminarPaquete(t_paquete* );
t_paquete* recibirPaquete(int );

#endif