#ifndef STORAGE_H_
#define STORAGE_H_

#include <utils/utils.h>
#include <utils/paquetes.h>
#include <utils/servidor.h>
#include <utils/serializacion.h>
#include <utils/estructuras.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <commons/crypto.h>
#include <commons/collections/dictionary.h>
#include <paths.h>

typedef struct{
    char* puerto;
    char* puntoMontaje;
    int retardoOperacion;
    int retardoAccesoBloque;
    bool freshStart;
    t_log_level nivelLog;
}storage_config;

typedef struct{
    int tam_bloque;
    int tam_fs;
    int cantidad_bloques;
}superblock_config;

extern t_log* logger;

void iniciarStorage(char* []);
void inicializarPaths();
void crearStorageConfig(char*);
void iniciarSuperblock();
void iniciarFileSystem();
void crearBitmap();
void crearBitArray(FILE*);
void crearHashIndex();
void crearPhysicalBlocks();
void crearEstructuraFile();
void crearArchivoInicial();
void cargarBitmapExistente();
int buscarBloqueLibre();
void agregarHardLink(int, char*, char*, int, int);
void eliminarHardLink(int, char*, char*, int, int);
void deduplicacionBloque(int,int, char*,char*, int, int);
void marcarBloqueOcupado(int,int);
void marcarBloqueOcupadoBitArray(int);
void liberarBloque(int,int);
void liberarBloqueBitArray(int);
char *calcularHash(int);
void guardarHashBloque(int bloque);
void eliminarHashBloque(int);
int buscarBloquePorHash(int);
void escribirBloque(t_instruccion*, int);
void* leerBloque(int);
void sumarWorker();
void restarWorker();
void* atenderWorker(void*);
int handshakeConWorker(int);
void enviarSizeBlock(int32_t, int);
void enviarRespuestaAWorker(int socket, codigo_respuesta_storage codigo, void* contenido,char*);
void finalizarStorage(int);
void retardo_operacion();
void retardo_bloque();
pthread_mutex_t* obtenerMutexFileTag(char*, char*);
void mutexDestroy(void* ptr);

#endif