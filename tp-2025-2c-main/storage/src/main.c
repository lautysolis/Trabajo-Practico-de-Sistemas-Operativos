#include "storage.h"

t_log* logger;
storage_config* configuracion;
superblock_config* superblock;

int main(int argc, char* argv[]) {
   
    int socketEscuchaStorage;

    iniciarStorage(argv);
    
    iniciarFileSystem();
    
    socketEscuchaStorage = iniciarServidor(configuracion->puerto);

    log_info(logger, "Storage escuchando en puerto: %s", configuracion->puerto);

    esperarCliente(socketEscuchaStorage,atenderWorker);
    
    finalizarStorage(socketEscuchaStorage);
    
    return 0;
}

