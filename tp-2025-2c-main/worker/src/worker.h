#ifndef WORKER_H_
#define WORKER_H_

#include <utils/paquetes.h>
#include <utils/cliente.h>
#include <utils/utils.h>
#include <utils/serializacion.h>
#include <utils/estructuras.h>


typedef struct
{
    char *ipMaster;
    char *puertoMaster;
    char *ipStorage;
    char *puertoStorage;
    int tamMemoria;
    int retardoMemoria;
    char *algoritmoRemplazo;
    char *pathQuerys;
    t_log_level nivelLog;
} t_worker_config;

typedef struct
{
    int socketStorage;
    int socketMaster;
} t_args_ejecucion;

typedef struct
{
    int marco;
    int precencia;
    int uso;
    int modificada;
} t_pagina;

typedef struct
{
    char *fileTag;
    t_list *paginas;
} t_tablaDePaginas;

typedef struct
{
    char *fileTag;
    t_pagina *pagina;
} t_paginaGlobal;

uint8_t *inicializarMemoriaInterna();

char* castearVoidString(void* ,int );

void procesarError(t_respuesta_storage *, char *, char *);
void confirmarDesalojo(int , char *, char *);
t_respuesta_storage *recibirRespuestaStorage();

int32_t handshakeConStorage(int32_t);
void enviarCodigoOperacion(int, codigo_operacion);
void enviarIdWorker(int32_t, int, codigo_operacion);
void iniciarWorker(char *[], int32_t *, t_worker_config **, t_log **);
t_worker_config *crearWorkerConfig(char *);
void liberarConfiguracionWorker(t_worker_config *);
void procesarError(t_respuesta_storage *, char *, char *);
t_respuesta_storage *recibirRespuestaStorage();

void destruirTablaDePaginas(void *);
void destruirTablaDePaginasAlFinalizar(void *);

void destruirPaginaGlobal(void *);

void finalizarWorker(int,t_log *, t_worker_config *, void *, t_bitarray *, t_list *, t_list *, pthread_mutex_t ,pthread_mutex_t ,sem_t);
void manejar_sigint(int);

// GLOBALES

extern t_log *logger;
extern t_worker_config *configuracion;
extern int interrupcion;
extern pthread_mutex_t mutex_interr;
extern pthread_mutex_t mutex_query;
extern int32_t idWorker;
extern int socketConMaster;
extern int socketConStorage;
extern int32_t sizeBlock;
extern int cantMarcos;
extern t_bitarray *bitmap;
extern t_list *tablas;
extern t_list *tablaPagGlobal;
extern uint8_t *memoriaInterna;
extern t_query *queryActual;
extern sem_t sem_query;
extern volatile sig_atomic_t terminar;
extern int puntero_clock;
extern char *bitarray;
extern pthread_t hiloEjecutor;

#endif