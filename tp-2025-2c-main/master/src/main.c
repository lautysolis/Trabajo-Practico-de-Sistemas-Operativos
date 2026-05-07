#include <master.h>
#include <planificacion.h>

t_log* logger;
master_config* configuracion;

int main(int argc, char* argv[]) {
    int socketEscuchaMaster;

    iniciarMaster(argv);

    socketEscuchaMaster = iniciarServidor(configuracion->puerto);
    
    log_info(logger, "## Master escuchando en puerto: %s", configuracion->puerto);


    iniciarPlanificacion();
    pthread_t readyHilo, exitHilo;

    pthread_create(&readyHilo, NULL, ready, NULL);
    pthread_detach(readyHilo);

    pthread_create(&exitHilo, NULL, exitStatus, NULL);
    pthread_detach(exitHilo);

    esperarCliente(socketEscuchaMaster,atenderCliente);
    
	finalizarMaster(socketEscuchaMaster,logger,configuracion);
    
    return 0;
}