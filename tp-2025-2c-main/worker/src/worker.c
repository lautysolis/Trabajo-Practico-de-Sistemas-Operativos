#include <worker.h>

char *castearVoidString(void *contenido, int contenidoLength)
{
    char *contenidoString = malloc(contenidoLength + 1);
    memcpy(contenidoString, contenido, contenidoLength);
    contenidoString[contenidoLength] = '\0';
    return contenidoString;
}

uint8_t *inicializarMemoriaInterna()
{
    uint8_t cero = 0;
    uint8_t *memoria = malloc(configuracion->tamMemoria);
    memset(memoria, cero, configuracion->tamMemoria);
    return memoria;
}

void procesarError(t_respuesta_storage *resp, char *file, char *tag)
{
    t_respuesta_query *respuesta = crearRespuestaQuery(resp->error, 0, file, tag, FIN_QUERY);
    pthread_mutex_lock(&mutex_query);
    liberarQuery(queryActual);
    queryActual = NULL;
    pthread_mutex_unlock(&mutex_query);
    enviarRespuestaQuery(respuesta, socketConMaster, WORKER);
    liberarRespuestaQuery(respuesta);
}

void confirmarDesalojo(int pc, char *file, char *tag)
{
    t_respuesta_query *respuesta = crearRespuestaQuery("Worker Desalojado", pc, file, tag, DESALOJO_QUERY);
    pthread_mutex_lock(&mutex_query);
    liberarQuery(queryActual);
    queryActual = NULL;
    pthread_mutex_unlock(&mutex_query);
    enviarRespuestaQuery(respuesta, socketConMaster, WORKER);
    liberarRespuestaQuery(respuesta);
}

t_respuesta_storage *recibirRespuestaStorage()
{
    t_paquete *paquete = recibirPaquete(socketConStorage);
    t_respuesta_storage *respuesta = deserializarRespuestaStorage(paquete->buffer);
    eliminarPaquete(paquete);
    return respuesta;
}

void destruirPagina(t_pagina *pagina)
{
    free(pagina);
}

void destruirTablaDePaginasAlFinalizar(void *ptr)
{
    t_tablaDePaginas *tabla = (t_tablaDePaginas *)ptr;

    list_destroy_and_destroy_elements(tabla->paginas, (void *)destruirPagina);

    free(tabla->fileTag);
    free(tabla);
}

void destruirTablaDePaginas(void *ptr)
{
    t_tablaDePaginas *tabla = (t_tablaDePaginas *)ptr;
    list_remove_element(tablas, tabla);
    destruirTablaDePaginasAlFinalizar(tabla);
}

void destruirPaginaGlobal(void *ptr)
{
    t_paginaGlobal *pagGlobal = (t_paginaGlobal *)ptr;
    free(pagGlobal->fileTag);
    free(pagGlobal);
}

// Handshake con servidor de storage
int32_t handshakeConStorage(int32_t idWorker)
{
    enviarIdWorker(idWorker, socketConStorage, HANDSHAKE_WORKER_STORAGE);
    t_paquete *paquete = recibirPaquete(socketConStorage);
    int32_t sizeBlock = deserializarSizeBlock(paquete->buffer);
    sizeBlock = (paquete->codigoOperacion == HANDSHAKE_WORKER_STORAGE) ? sizeBlock : -1;
    eliminarPaquete(paquete);
    return sizeBlock;
}

// Envia codigo de operacion
void enviarCodigoOperacion(int socket, codigo_operacion codigoOperacion)
{
    t_buffer *buffer = crearBuffer(sizeof(int32_t));
    enviarPaquete(socket, codigoOperacion, buffer);
}

// Envia el id del worker
void enviarIdWorker(int32_t idWorker, int socket, codigo_operacion codigo)
{
    t_buffer *buffer = serializarIdWorker(idWorker);
    enviarPaquete(socket, codigo, buffer);
}

// Libera memoria de la estructura que contiene los datos de configuracion
void liberarConfiguracionWorker(t_worker_config *configuracion)
{
    free(configuracion->ipMaster);
    free(configuracion->puertoMaster);
    free(configuracion->ipStorage);
    free(configuracion->puertoStorage);
    free(configuracion->algoritmoRemplazo);
    free(configuracion->pathQuerys);
    free(configuracion);
}

// Crea la estructura con los datos del archivo de configuracion
t_worker_config *crearWorkerConfig(char *archivoConfig)
{
    t_config *config = iniciar_config(archivoConfig);

    char *ipMaster = config_get_string_value(config, "IP_MASTER");
    char *puertoMaster = config_get_string_value(config, "PUERTO_MASTER");
    char *ipStorage = config_get_string_value(config, "IP_STORAGE");
    char *puertoStorage = config_get_string_value(config, "PUERTO_STORAGE");
    int tamMemoria = config_get_int_value(config, "TAM_MEMORIA");
    int retardoMemoria = config_get_int_value(config, "RETARDO_MEMORIA");
    char *algoritmoRemplazo = config_get_string_value(config, "ALGORITMO_REEMPLAZO");
    char *pathQuerys = config_get_string_value(config, "PATH_QUERYS");
    char *nivelString = config_get_string_value(config, "LOG_LEVEL");
    t_log_level nivelLog = log_level_from_string(nivelString);

    t_worker_config *configuracion = malloc(sizeof(t_worker_config));
    configuracion->ipMaster = malloc(strlen(ipMaster) + 1);
    memcpy(configuracion->ipMaster, ipMaster, strlen(ipMaster) + 1);
    configuracion->puertoMaster = malloc(strlen(puertoMaster) + 1);
    memcpy(configuracion->puertoMaster, puertoMaster, strlen(puertoMaster) + 1);
    configuracion->ipStorage = malloc(strlen(ipStorage) + 1);
    memcpy(configuracion->ipStorage, ipStorage, strlen(ipStorage) + 1);
    configuracion->puertoStorage = malloc(strlen(puertoStorage) + 1);
    memcpy(configuracion->puertoStorage, puertoStorage, strlen(puertoStorage) + 1);
    configuracion->algoritmoRemplazo = malloc(strlen(algoritmoRemplazo) + 1);
    memcpy(configuracion->algoritmoRemplazo, algoritmoRemplazo, strlen(algoritmoRemplazo) + 1);
    configuracion->pathQuerys = malloc(strlen(pathQuerys) + 1);
    memcpy(configuracion->pathQuerys, pathQuerys, strlen(pathQuerys) + 1);
    configuracion->tamMemoria = tamMemoria;
    configuracion->retardoMemoria = retardoMemoria;
    configuracion->nivelLog = nivelLog;

    config_destroy(config);

    return configuracion;
}

// Inicializa el worker
void iniciarWorker(char *argv[], int32_t *idWorker, t_worker_config **configuracion, t_log **logger)
{
    char *archivoConfig = argv[1];
    *idWorker = validarEntero(argv[2], "ERROR: el segundo argumento debe ser un entero.");
    *configuracion = crearWorkerConfig(archivoConfig);
    *logger = log_create("worker.log", "Worker", true, (*configuracion)->nivelLog);
}

// Finalizar Worker
void finalizarWorker(int socketConStorage, t_log *logger, t_worker_config *configuracion, void *memoriaInterna, t_bitarray *bitmap, t_list *tablas, t_list *tablaPagGlobal, pthread_mutex_t mutex_interr, pthread_mutex_t mutex_query, sem_t sem_query)
{
    liberarConfiguracionWorker(configuracion);
    free(memoriaInterna);
    list_destroy_and_destroy_elements(tablas, destruirTablaDePaginasAlFinalizar);
    list_destroy_and_destroy_elements(tablaPagGlobal, destruirPaginaGlobal);
    bitarray_destroy(bitmap);
    free(bitarray);
    pthread_mutex_destroy(&mutex_interr);
    pthread_mutex_destroy(&mutex_query);
    sem_destroy(&sem_query);

    log_info(logger, "## Finaliza el Worker");
    log_destroy(logger);
    close(socketConStorage);
    exit(0);
}

void manejar_sigint(int sig)
{
    (void)sig;
    terminar = 1;
    close(socketConMaster);
    sem_post(&sem_query);
}