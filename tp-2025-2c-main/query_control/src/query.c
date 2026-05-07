#include <query.h>

void liberarConfiguracionQuery(query_config* configuracion){
	free(configuracion->ipMaster);
    free(configuracion->puertoMaster);
	free(configuracion);
}

query_config* crearQueryControlConfig(char* archivoConfig){
    t_config* config = iniciar_config(archivoConfig);

	char* ipMaster = config_get_string_value(config,"IP_MASTER");
	char* puertoMaster = config_get_string_value(config,"PUERTO_MASTER");
	char* nivelString = config_get_string_value(config,"LOG_LEVEL");
    t_log_level nivelLog = log_level_from_string(nivelString);

    query_config* configuracion = malloc(sizeof(query_config));
    configuracion->ipMaster = malloc(strlen(ipMaster)+1);
    memcpy(configuracion->ipMaster, ipMaster, strlen(ipMaster) + 1);
    configuracion->puertoMaster = malloc(strlen(puertoMaster)+1);
    memcpy(configuracion->puertoMaster, puertoMaster, strlen(puertoMaster) + 1);
    configuracion->nivelLog = nivelLog;

    config_destroy(config);

    return configuracion;
}

void iniciarQueryControl(char* argv[],query_config** configuracion,t_query** query){
    char* archivoConfig = argv[1];
    char* archivoQuery = argv[2];
    int32_t prioridad = validarEntero(argv[3],"ERROR: el tercer argumento debe ser un entero.");
    *query = crearQuery(archivoQuery, prioridad);
    *configuracion = crearQueryControlConfig(archivoConfig);
    logger = log_create("query.log","QueryControl",true,(*configuracion)->nivelLog);
}

void finalizarQueryControl(int socket, query_config* configuracion, t_query* query){	
    liberarQuery(query);
    liberarConfiguracionQuery(configuracion);
    close(socket);
    log_info(logger,"## Finalizando Query Control");
    log_destroy(logger);
	exit(0);
}
