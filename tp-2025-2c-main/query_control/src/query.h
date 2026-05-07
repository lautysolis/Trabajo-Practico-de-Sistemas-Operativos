#ifndef QUERY_H
#define QUERY_H

#include <utils/cliente.h>
#include <utils/paquetes.h>
#include <utils/serializacion.h>
#include <utils/estructuras.h>

extern t_log* logger;

typedef struct {
	char* ipMaster;
    char* puertoMaster;
    t_log_level nivelLog;
} query_config;


int handshakeConMaster(int);
t_query* crearQuery(char*,int);
void liberarQuery(t_query*);
void liberarConfiguracionQuery(query_config*);
query_config* crearQueryControlConfig(char*);
void finalizarQueryControl(int, query_config*, t_query* );
void iniciarQueryControl(char* [],query_config**,t_query**);

#endif