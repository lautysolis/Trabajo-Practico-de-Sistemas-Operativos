#include <master.h>

void liberarConfiguracionMaster(master_config* configuracion){
	free(configuracion->puerto);
    free(configuracion->algoritmoPlanificacion);
	free(configuracion);
}

void crearMasterConfig(char* archivoConfig){
    t_config* config = iniciar_config(archivoConfig);

    char* puerto = config_get_string_value(config, "PUERTO_ESCUCHA");
    char* algoritmoPlanificacion = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
    int tiempoAging = config_get_int_value(config, "TIEMPO_AGING"); 
    char* nivelString = config_get_string_value(config, "LOG_LEVEL");
    t_log_level nivelLog = log_level_from_string(nivelString);

    configuracion = malloc(sizeof(master_config));
    configuracion->puerto = malloc(strlen(puerto)+1);
    memcpy(configuracion->puerto, puerto, strlen(puerto) + 1);
    configuracion->algoritmoPlanificacion = malloc(strlen(algoritmoPlanificacion)+1);
    memcpy(configuracion->algoritmoPlanificacion, algoritmoPlanificacion, strlen(algoritmoPlanificacion) + 1);
    
    configuracion->tiempoAging = tiempoAging;
    configuracion->nivelLog = nivelLog;

    config_destroy(config);    
}

void iniciarMaster(char* argv[]){
    char* archivoConfig = argv[1];
    crearMasterConfig(archivoConfig);
    logger = log_create("master.log","Master",true,configuracion->nivelLog);
}

void* atenderCliente(void* conexion){

    int socketCliente = *((int*)conexion);
    free(conexion); 

    t_paquete* paquete = recibirPaquete(socketCliente);

    switch(paquete->codigoOperacion) {
        case QUERY:
            atenderQuery(paquete,socketCliente);
            break;
        case WORKER: 
            atenderWorker(paquete,socketCliente);
            break;
        default:
            break;
    }
    return NULL;
}

void finalizarMaster(int socket, t_log* logger, master_config* configuracion){	
    liberarConfiguracionMaster(configuracion);
	log_destroy(logger);
    close(socket);
}