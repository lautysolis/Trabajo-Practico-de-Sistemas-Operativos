#ifndef UTILS_ESTRUCTURAS_H_
#define UTILS_ESTRUCTURAS_H_

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
#include <utils/paquetes.h>

typedef enum{
    CREACION_QUERY,
    ESPERA_QUERY,
    EJECUCION_QUERY,
    RESPUESTA_QUERY,
    DESALOJO_QUERY,
    FIN_QUERY,
}codigo_accion_query;

typedef struct {
    int32_t id;
    char* archivoQuery;
	int32_t archivoQueryLength;
    int32_t prioridad;
    int32_t pc;
    codigo_accion_query codAccion;
} t_query;

typedef struct {
    t_query* query;
    int32_t socketQueryControl;
    bool desconexionQueryControl;
    pthread_mutex_t mutexPrioridad;
    pthread_mutex_t mutexCodAccion;
    pthread_mutex_t mutexDesconexionQueryControl;
    codigo_accion_query codAccion;
    pthread_t hiloAging;
    bool agingIniciado;
} t_query_master;

typedef struct {
    int32_t id;
    int32_t idQuery;
    int32_t socketWorker;
} t_worker;

typedef struct {
    codigo_accion_query codRespuesta;
    int32_t pc;
    char* mensaje;
    int32_t mensajeLenght;
    char* nombreFile;
    int32_t nombreFileLenght;
    char* tag;
    int32_t tagLenght;
} t_respuesta_query;

typedef enum{
    CREATE,
    TRUNCATE,
    TAG,
    COMMIT,
    FLUSH,
    DELETE,
    END,
    READ,
    WRITE,
    WRITE_BLOCK,
    READ_BLOCK
}codigo_instruccion;

typedef struct{
    int32_t queryID;
    codigo_instruccion codigoIns;
    int32_t fileTagLength;     
    char* fileTag;              // Para usar en Worker
    int32_t nombreFileLength;
    char* nombreFile;           // En caso de instruccion TAG equivale a NOMBRE_FILE_ORIGEN
    int32_t tagLength;
    char* tag;                  // En caso de instruccion TAG equivale a TAG_ORIGEN
    int32_t tamanio;            // Solo para: TRUNCATE, READ
    int32_t direccionBase;      // Solo para: WRITE, READ
    int32_t contenidoLength;
    void* contenido;            // Solo para: WRITE
    int32_t nombreFileDestinoLength;
    char* nombreFileDestino;    // Solo para: TAG
    int32_t tagDestinoLength;
    char* tagDestino;           // Solo para: TAG
    int32_t nroBloque;          // Para leer o escribir los bloques en storage
}t_instruccion;

typedef enum{
    CORRECTO,
    ERROR_INEXISTENTE,
    ERROR_PREEXISTENTE,
    ERROR_INSUFICIENTE,
    ERROR_PROHIBIDO,
    ERROR_LIMITE,
}codigo_respuesta_storage;

typedef struct {
    codigo_respuesta_storage codRespuesta;
    void* contenido;
    int32_t contenidoLength;
    char* error;
    int32_t errorLength;
}t_respuesta_storage;

t_query* crearQuery(char*,int);
void enviarQuery(t_query*,int,codigo_operacion);
void liberarQuery(t_query*);
t_query_master* crearQueryMaster(t_query*,int);
void liberarQueryMaster(t_query_master*);
t_respuesta_query* crearRespuestaQuery(char*,int,char*,char*,codigo_accion_query);
void enviarRespuestaQuery(t_respuesta_query*,int,codigo_operacion);
void liberarRespuestaQuery(t_respuesta_query*);
t_instruccion* crearInstruccion(int,codigo_instruccion,char* ,char* ,char* ,int , int ,void* , int, char* ,char* , int);
void enviarInstruccion(t_instruccion* ,int);
void liberarInstruccion(t_instruccion* );
t_worker* crearWorker(int,int);
void liberarWorker(t_worker* worker);
t_respuesta_storage* crearRespuestaStorage(codigo_respuesta_storage,void*,char*,int);
void enviarRespuestaStorage(t_respuesta_storage*,int);
void liberarRespuestaStorage(t_respuesta_storage*);
#endif 