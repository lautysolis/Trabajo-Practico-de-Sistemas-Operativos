#ifndef MASTER_H_
#define MASTER_H_

#include <utils/paquetes.h>
#include <utils/servidor.h>
#include <utils/estructuras.h>
#include <utils/utils.h>
#include <utils/serializacion.h>

typedef struct 
{
    char* puerto;
    char* algoritmoPlanificacion;
    int tiempoAging;
    t_log_level nivelLog;
}master_config;


extern t_log* logger;
extern master_config* configuracion;

void* atenderCliente(void*);
void atenderQuery(t_paquete*,int);
void atenderWorker(t_paquete*,int);
void finalizarMaster(int, t_log*, master_config*);
void liberarConfiguracionMaster(master_config*);
void crearMasterConfig(char*);
void iniciarMaster(char*[]);


#endif
