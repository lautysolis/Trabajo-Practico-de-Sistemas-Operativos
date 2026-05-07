#include <worker.h>
#include <ejecucion.h>
#include <memoria.h>
#include <signal.h>

t_log *logger;
t_worker_config *configuracion = NULL;
int interrupcion = 0;
pthread_mutex_t mutex_interr;
pthread_mutex_t mutex_query;
int32_t idWorker;
int socketConMaster;
int socketConStorage;
int32_t sizeBlock;
int cantMarcos;
t_bitarray *bitmap;
t_list *tablas;
t_list *tablaPagGlobal;
uint8_t *memoriaInterna;
t_query *queryActual;
sem_t sem_query;
volatile sig_atomic_t terminar = 0;
int puntero_clock = 0;
char *bitarray;
pthread_t hiloEjecutor;

int main(int argc, char *argv[])
{   
    signal(SIGINT, manejar_sigint);
    iniciarWorker(argv, &idWorker, &configuracion, &logger);

    socketConStorage = iniciarCliente(configuracion->ipStorage, configuracion->puertoStorage);

    sizeBlock = handshakeConStorage(idWorker);

    log_info(logger, "## Conexión al storage exitosa. IP: %s, Puerto: %s", configuracion->ipStorage, configuracion->puertoStorage);
    log_info(logger, "## Recibi el Size Block: %i", sizeBlock);

    cantMarcos = configuracion->tamMemoria / sizeBlock;

    memoriaInterna = inicializarMemoriaInterna();

    bitmap = crearBitmapWorker();

    tablas = list_create();

    tablaPagGlobal = list_create();

    socketConMaster = iniciarCliente(configuracion->ipMaster, configuracion->puertoMaster);
    log_info(logger, "## Conexión al Master exitosa. IP: %s, Puerto: %s", configuracion->ipMaster, configuracion->puertoMaster);

    enviarIdWorker(idWorker, socketConMaster, WORKER);


    pthread_mutex_init(&mutex_interr, NULL);
    pthread_mutex_init(&mutex_query, NULL);
    sem_init(&sem_query, 0, 0);


    pthread_create(&hiloEjecutor, NULL, queryInterpreter, NULL);

    escucharMaster();
    pthread_join(hiloEjecutor, NULL);

    finalizarWorker(socketConStorage, logger, configuracion, memoriaInterna, bitmap, tablas, tablaPagGlobal,mutex_interr,mutex_query,sem_query);

    return 0;
}
