#include <query.h>

t_log* logger;

int main(int argc, char* argv[]){
    query_config* configuracion = NULL;
    t_query* query = NULL;
    int socketConMaster;
    //int result;

    iniciarQueryControl(argv,&configuracion,&query);
    
    socketConMaster = iniciarCliente(configuracion->ipMaster, configuracion->puertoMaster);
    
    log_info(logger,"## Conexión al Master exitosa. IP: %s, Puerto: %s", configuracion->ipMaster, configuracion->puertoMaster);

    enviarQuery(query, socketConMaster,QUERY);
    
    log_info(logger,"## Solicitud de ejecución de Query: %s, prioridad: %d", query->archivoQuery, query->prioridad);
    
    t_paquete* paqueteWorker;
    t_respuesta_query* respuestaQuery;
    int fin = 0;
    while(fin==0){
        paqueteWorker = recibirPaquete(socketConMaster);
        if(paqueteWorker==NULL){
            finalizarQueryControl(socketConMaster,configuracion,query);
            break;
        }
        respuestaQuery = deserializarRespuestaQuery(paqueteWorker->buffer);
        eliminarPaquete(paqueteWorker);
        switch (respuestaQuery->codRespuesta){
            case RESPUESTA_QUERY:
                log_info(logger,"## Lectura realizada: File %s:%s, contenido: %s",respuestaQuery->nombreFile,respuestaQuery->tag,respuestaQuery->mensaje);
                break;
            case FIN_QUERY:
                log_info(logger,"## Query Finalizada - %s",respuestaQuery->mensaje);
                fin=1;
                break;
            default:
                break;
        }
        liberarRespuestaQuery(respuestaQuery);
    }
    
    return 0;
}
