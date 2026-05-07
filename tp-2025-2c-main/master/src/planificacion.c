#include <planificacion.h>

//SEMAFOROS
sem_t semQueryEsperandoReady;
sem_t semQueryReady;
sem_t semQueryExecute;
sem_t semQueryExit;

//MUTEX
pthread_mutex_t mutexColaNew;
pthread_mutex_t mutexColaDesalojo;
pthread_mutex_t mutexListaReady;
pthread_mutex_t mutexListaExecute;
pthread_mutex_t mutexColaExit;
pthread_mutex_t mutexListaWorker;
pthread_mutex_t mutexUltimoQueryId;

//COLLECTIONS GLOBALES
t_queue* colaNew;
t_queue* colaDesalojo;
t_list* listaReady;
t_list* listaExecute;
t_queue* colaExit;
t_list* listaWorker;

//VARIABLES GLOBALES
int ultimoQueryId;

void iniciarPlanificacion(void){
    sem_init(&semQueryEsperandoReady, 0, 0);
    sem_init(&semQueryReady, 0, 0);
    sem_init(&semQueryExecute, 0, 0);
    sem_init(&semQueryExit, 0, 0);
    pthread_mutex_init(&mutexColaNew, NULL);
    pthread_mutex_init(&mutexColaDesalojo, NULL);
    pthread_mutex_init(&mutexListaReady, NULL);
    pthread_mutex_init(&mutexListaExecute, NULL);
    pthread_mutex_init(&mutexColaExit, NULL);
    pthread_mutex_init(&mutexListaWorker, NULL);
    pthread_mutex_init(&mutexUltimoQueryId, NULL);
    colaNew = queue_create();
    colaDesalojo = queue_create();
    listaReady = list_create();
    listaExecute = list_create();
    colaExit = queue_create();
    listaWorker = list_create();
    ultimoQueryId = 0;
}

void atenderQuery(t_paquete* paquete,int socketQueryControl){
    t_query* query = deserializarQuery(paquete->buffer);
    eliminarPaquete(paquete);
    t_query_master* queryMaster = crearQueryMaster(query,socketQueryControl);
    asignarQueryId(queryMaster);
    int idQuery = queryMaster->query->id;
    int nivel = obtenerGradoMultiprocesamiento();
    int prioridad = obtenerPrioridadQuery(queryMaster);
    log_info(logger,"## Se conecta un Query Control para ejecutar la Query %s con prioridad %i - Id asignado: %i Nivel multiprocesamiento %i",queryMaster->query->archivoQuery,prioridad,queryMaster->query->id,nivel);
    agregarQueryMasterCola(&mutexColaNew,colaNew,queryMaster);
    sem_post(&semQueryEsperandoReady);
    if(recibirPaquete(socketQueryControl)==NULL){
        manejarDesconexionQueryControl(queryMaster,idQuery);
    }
}

void manejarDesconexionQueryControl(t_query_master* queryMaster,int idQuery) {
    if(obtenerWorkerEjecutandoQuery(idQuery) != NULL){
        int nivel = obtenerGradoMultiprocesamiento();
        int prioridad = obtenerPrioridadQuery(queryMaster);
        desconectarQueryControl(queryMaster);
        enviarDesalojo(queryMaster);
        log_info(logger,"## Se desconecta un Query Control. Se finaliza la Query %i con prioridad %i. Nivel multiprocesamiento %i",queryMaster->query->id,prioridad,nivel);
    }else if(obtenerQueryMasterDeReadyPorId(idQuery)!=NULL){
        int nivel = obtenerGradoMultiprocesamiento();
        int prioridad = obtenerPrioridadQuery(queryMaster);
        desconectarQueryControl(queryMaster);
        sacarQueryMasterListaPasadoPorParametro(&mutexListaReady,listaReady,queryMaster);
        cambiarCodAccionQuery(queryMaster,FIN_QUERY);
        agregarQueryMasterCola(&mutexColaExit,colaExit,queryMaster);
        log_info(logger,"## Se desconecta un Query Control. Se finaliza la Query %i con prioridad %i. Nivel multiprocesamiento %i",queryMaster->query->id,prioridad,nivel);
        sem_post(&semQueryExit);
    }
}

void* ready(void* atributo){
    while(true){
        sem_wait(&semQueryEsperandoReady);
        t_query_master* queryMaster = obtenerSiguienteQueryMasterReady();

        agregarQueryMasterLista(&mutexListaReady,listaReady,queryMaster);
        
        cambiarCodAccionQuery(queryMaster,ESPERA_QUERY);

        verificarDesalojo(queryMaster);

        comenzarAging(queryMaster);

        sem_post(&semQueryReady);
    }
    return NULL;
}

void comenzarAging(t_query_master* queryMaster){
    if(configuracion->tiempoAging>0 && strcmp(configuracion->algoritmoPlanificacion,"FIFO")!=0 && !queryMaster->agingIniciado){
        queryMaster->agingIniciado = true;
        pthread_create(&queryMaster->hiloAging, NULL, aging, queryMaster);
    }
}

void* aging(void* queryMasterVoid){
    t_query_master* queryMaster = (t_query_master*) queryMasterVoid;

    while(!queryControlDesconectado(queryMaster) && obtenerCodAccion(queryMaster) != FIN_QUERY && obtenerWorkerEjecutandoQuery(queryMaster->query->id) == NULL && obtenerPrioridadQuery(queryMaster) > 0 ){
        usleep(configuracion->tiempoAging * 1000);
        if(!queryControlDesconectado(queryMaster) && obtenerCodAccion(queryMaster) != FIN_QUERY && obtenerWorkerEjecutandoQuery(queryMaster->query->id) == NULL && obtenerPrioridadQuery(queryMaster) > 0){
            disminuirPrioridadQuery(queryMaster);
            ordenarListaQuerysMaster(&mutexListaReady,listaReady);
            verificarDesalojo(queryMaster);
        }
    }           
    queryMaster->agingIniciado = false;
    return NULL;
}

void verificarDesalojo(t_query_master* queryMaster){
    if(strcmp(configuracion->algoritmoPlanificacion,"FIFO")!=0){
        int lenghtListaExecute = lengthLista(&mutexListaExecute,listaExecute);
        if(lenghtListaExecute>0 && todosLosWorkersTienenQuery()){
            t_query_master* queryMasterExecuteMenorPrioridad = queryMasterListaSegunPosicion(&mutexListaExecute,listaExecute,lengthLista(&mutexListaExecute,listaExecute)-1);
            if(obtenerPrioridadQuery(queryMaster) < obtenerPrioridadQuery(queryMasterExecuteMenorPrioridad)){
                enviarDesalojo(queryMasterExecuteMenorPrioridad);
            }
        }
    }
}

void enviarDesalojo(t_query_master* queryMaster){
    t_worker* worker = obtenerWorkerEjecutandoQuery(queryMaster->query->id);
    cambiarCodAccionQuery(queryMaster,DESALOJO_QUERY);
    enviarQuery(queryMaster->query,worker->socketWorker,MASTER);
}

void* exitStatus(void* atributo){
    while(true){
        sem_wait(&semQueryExit);
        t_query_master* queryMaster = sacarQueryMasterCola(&mutexColaExit,colaExit);
        liberarQueryMaster(queryMaster);
    }
    return NULL;
}

void atenderWorker(t_paquete* paquete,int socketWorker){
    int idWorker = deserializarIdWorker(paquete->buffer);
    eliminarPaquete(paquete);
    t_query_master* queryMaster;
    t_paquete* paqueteWorker;
    t_respuesta_query* respuestaQuery;
    t_worker* worker = crearWorker(idWorker,socketWorker);
    agregarWorkerLista(worker);
    int nivel = obtenerGradoMultiprocesamiento();
    log_info(logger,"## Se conecta el Worker %i - Cantidad total de Workers: %i",idWorker,nivel);
    bool workerFuncionando = true;
    while(workerFuncionando){
        sem_wait(&semQueryReady);
        queryMaster = sacarQueryMasterLista(&mutexListaReady,listaReady,0);
        agregarQueryMasterLista(&mutexListaExecute,listaExecute,queryMaster);
        cambiarCodAccionQuery(queryMaster,EJECUCION_QUERY);
        worker->idQuery = queryMaster->query->id;
        enviarQuery(queryMaster->query,worker->socketWorker,MASTER);
        int prioridad = obtenerPrioridadQuery(queryMaster);
        log_info(logger,"## Se envía la Query %i (%i) al Worker %i",queryMaster->query->id,prioridad,worker->id);
        while(worker->idQuery>=0){        
            paqueteWorker = recibirPaquete(worker->socketWorker);
            if(!paqueteWorker){
                manejarDesconexionWorker(queryMaster,worker);
                workerFuncionando = false;
                break;
            }
            respuestaQuery = deserializarRespuestaQuery(paqueteWorker->buffer);
            eliminarPaquete(paqueteWorker);
            cambiarCodAccionQuery(queryMaster,respuestaQuery->codRespuesta);
            switch (respuestaQuery->codRespuesta)
            {
                case DESALOJO_QUERY:
                    desalojarQuery(queryMaster,respuestaQuery,idWorker);
                    worker->idQuery = -1;
                    break;
                case RESPUESTA_QUERY:
                    log_info(logger,"## Se envía un mensaje de lectura de la Query %i en el Worker %i al Query Control",queryMaster->query->id,idWorker);
                    enviarRespuestaQuery(respuestaQuery,queryMaster->socketQueryControl,MASTER);
                    cambiarCodAccionQuery(queryMaster,EJECUCION_QUERY);
                    break;
                case FIN_QUERY:
                    worker->idQuery = -1;
                    char* motivoFin = string_from_format("## Se terminó la Query %i en el Worker %i",queryMaster->query->id,idWorker);
                    enviarExit(queryMaster,respuestaQuery,idWorker,motivoFin);
                    free(motivoFin);
                    break;
                default:
                    break;
            }
            liberarRespuestaQuery(respuestaQuery);
        }
    }
}

void manejarDesconexionWorker(t_query_master* queryMaster,t_worker* worker) {
    eliminarWorkerLista(worker);
    int nivel = obtenerGradoMultiprocesamiento();
    char* motivoFin = string_from_format("## Se desconecta el Worker %i - Se finaliza la Query %i - Cantidad total de Workers: %i",worker->id,queryMaster->query->id,nivel);
    t_respuesta_query* respuestaQuery = crearRespuestaQuery("Desconexión de worker",0,"","",FIN_QUERY);
    enviarExit(queryMaster,respuestaQuery,worker->id,motivoFin);
    liberarWorker(worker);
    liberarRespuestaQuery(respuestaQuery);
    free(motivoFin);
}

void desalojarQuery(t_query_master* queryMaster,t_respuesta_query* respuestaQuery,int idWorker){
    queryMaster->query->pc = respuestaQuery->pc;
    sacarQueryMasterListaPasadoPorParametro(&mutexListaExecute,listaExecute,queryMaster);
    int prioridad = obtenerPrioridadQuery(queryMaster);
    if(!queryControlDesconectado(queryMaster)){
        agregarQueryMasterCola(&mutexColaDesalojo,colaDesalojo,queryMaster);
        cambiarCodAccionQuery(queryMaster,ESPERA_QUERY);
        log_info(logger,"## Se desaloja la Query %i (%i) del Worker %i - Motivo: PRIORIDAD",queryMaster->query->id,prioridad,idWorker);
        sem_post(&semQueryEsperandoReady);
    }else{
        cambiarCodAccionQuery(queryMaster,FIN_QUERY);
        agregarQueryMasterCola(&mutexColaExit,colaExit,queryMaster);
        log_info(logger,"## Se desaloja la Query %i (%i) del Worker %i - Motivo: DESCONEXION",queryMaster->query->id,prioridad,idWorker);
        sem_post(&semQueryExit);
    }
}

void enviarExit(t_query_master* queryMaster,t_respuesta_query* respuestaQuery,int idWorker,char* motivoFin){
    enviarRespuestaQuery(respuestaQuery,queryMaster->socketQueryControl,MASTER);
    sacarQueryMasterListaPasadoPorParametro(&mutexListaExecute,listaExecute,queryMaster);
    agregarQueryMasterCola(&mutexColaExit,colaExit,queryMaster);
    cambiarCodAccionQuery(queryMaster,FIN_QUERY);
    log_info(logger,"%s",motivoFin);
    sem_post(&semQueryExit);
}

//FUNCION: Obtiene la siguiente query que se encolara en ready
t_query_master* obtenerSiguienteQueryMasterReady(){ 
    t_query_master* queryMaster = NULL; 
    t_queue* cola = NULL; 
    pthread_mutex_t* mutex = NULL; 
    
    pthread_mutex_lock(&mutexColaDesalojo);
    bool colaDesalojoNoEstaVacia = !queue_is_empty(colaDesalojo);
    pthread_mutex_unlock(&mutexColaDesalojo);
    if(colaDesalojoNoEstaVacia){ 
        cola = colaDesalojo; 
        mutex = &mutexColaDesalojo;     
    }else{ 
        cola = colaNew; 
        mutex = &mutexColaNew; 
    } 
    queryMaster = sacarQueryMasterCola(mutex,cola); 
    return queryMaster; 
}

//MONITOR: Asignar id a cada nueva query (mutua exclusion)
void asignarQueryId(t_query_master* queryMaster){
    pthread_mutex_lock(&mutexUltimoQueryId);
    queryMaster->query->id = ultimoQueryId;
    ultimoQueryId++;
    pthread_mutex_unlock(&mutexUltimoQueryId);
}

//MONITOR: obteniene el grado de multiprocesamiento, cantidad de workers conectados(mutua exclusion)
int obtenerGradoMultiprocesamiento(void){
    int nivel;
    nivel = lengthLista(&mutexListaWorker,listaWorker);
    return nivel;
}

//MONITOR: obteniene el grado de multiprocesamiento, cantidad de workers conectados(mutua exclusion)
int obtenerPrioridadQuery(t_query_master* queryMaster){
    int prioridad;
    pthread_mutex_lock(&(queryMaster->mutexPrioridad));
    prioridad = queryMaster->query->prioridad;
    pthread_mutex_unlock(&(queryMaster->mutexPrioridad));
    return prioridad;
}

void disminuirPrioridadQuery(t_query_master* queryMaster){
    pthread_mutex_lock(&queryMaster->mutexPrioridad);
    if (queryMaster->query->prioridad > 0) {
        log_info(logger,"## %i Cambio de prioridad: %i - %i",queryMaster->query->id,queryMaster->query->prioridad,queryMaster->query->prioridad-1);
        queryMaster->query->prioridad--;
    }
    pthread_mutex_unlock(&queryMaster->mutexPrioridad);
}

void cambiarCodAccionQuery(t_query_master* queryMaster,codigo_accion_query codAccion){
    pthread_mutex_lock(&queryMaster->mutexCodAccion);
    queryMaster->query->codAccion = codAccion;
    pthread_mutex_unlock(&queryMaster->mutexCodAccion);
}

codigo_accion_query obtenerCodAccion(t_query_master* queryMaster){
    pthread_mutex_lock(&queryMaster->mutexCodAccion);
    codigo_accion_query codAccion = queryMaster->query->codAccion;
    pthread_mutex_unlock(&queryMaster->mutexCodAccion);
    return codAccion;
}

int lengthLista(pthread_mutex_t* mutexLista,t_list* lista){
    int length;
    pthread_mutex_lock(mutexLista);
    length = list_count_satisfying(lista, siempreTrue);
    pthread_mutex_unlock(mutexLista);
    return length;
}

//FUNCION: siempre devuelve true para obtener cantidad total de workers conectados
bool siempreTrue(void* parametro) {
    return true;
}

//MONITOR: Aumentar grado de multiprocesamieto, agregando worker a lista de workers (mutua exclusion)
void agregarWorkerLista(t_worker* worker){
    pthread_mutex_lock(&mutexListaWorker);
    list_add(listaWorker,worker);
    pthread_mutex_unlock(&mutexListaWorker);
}

//MONITOR: disminuir grado de multiprocesamieto, agregando worker a lista de workers (mutua exclusion)
void eliminarWorkerLista(t_worker* worker){
    pthread_mutex_lock(&mutexListaWorker);
    list_remove_element(listaWorker, worker);
    pthread_mutex_unlock(&mutexListaWorker);
}

//MONITOR: Obtiene que worker esta ejecutando la query o si nadie la esta ejecutando NULL
t_worker* obtenerWorkerEjecutandoQuery(int idQuery) {
    bool ejecutaQuery(void* workerVoid) {
        t_worker* worker = (t_worker*) workerVoid;
        return worker->idQuery == idQuery;
    }
    pthread_mutex_lock(&mutexListaWorker);
    t_worker* worker = (t_worker*)list_find(listaWorker,ejecutaQuery);
    pthread_mutex_unlock(&mutexListaWorker);
    return worker;
}

//MONITOR: Agregar query a una cola (mutua exclusion)
void agregarQueryMasterCola(pthread_mutex_t* mutexCola,t_queue* cola,t_query_master* queryMaster){
    pthread_mutex_lock(mutexCola);
    queue_push(cola,queryMaster);
    pthread_mutex_unlock(mutexCola);
}

//MONITOR: Saca query de una cola (mutua exclusion)
t_query_master* sacarQueryMasterCola(pthread_mutex_t* mutexCola,t_queue* cola){
    pthread_mutex_lock(mutexCola);
    void* queryMasterVoid = queue_pop(cola);
    pthread_mutex_unlock(mutexCola);
    t_query_master* queryMaster = (t_query_master*) queryMasterVoid;
    return queryMaster;
}

//MONITOR: Agrega una query a una lista dependiendo del algoritmo (mutua exclusion)
void agregarQueryMasterLista(pthread_mutex_t* mutexLista,	t_list* lista,t_query_master* queryMaster){
    if(strcmp(configuracion->algoritmoPlanificacion,"FIFO")==0){
        pthread_mutex_lock(mutexLista);
        list_add(lista,queryMaster);
        pthread_mutex_unlock(mutexLista);
    }else{
        pthread_mutex_lock(mutexLista);
        list_add_sorted(lista,queryMaster,mayorPrioridad);
        pthread_mutex_unlock(mutexLista);
    }
}

//FUNCION: Ordenar por mayor prioridad -> menos numero de prioridad
bool mayorPrioridad(void* a, void* b) {
    t_query_master* queryMasterA = (t_query_master*) a;
    t_query_master* queryMasterB = (t_query_master*) b;
    return obtenerPrioridadQuery(queryMasterA) < obtenerPrioridadQuery(queryMasterB);
}

//MONITOR: Saca query de una lista segun el index (mutua exclusion)
t_query_master* sacarQueryMasterLista(pthread_mutex_t* mutexLista,t_list* lista,int index){
    pthread_mutex_lock(mutexLista);
    void* queryMasterVoid = list_remove(lista,index);
    pthread_mutex_unlock(mutexLista);
    t_query_master* queryMaster = (t_query_master*) queryMasterVoid;
    return queryMaster;
}

void sacarQueryMasterListaPasadoPorParametro(pthread_mutex_t* mutexLista,t_list* lista,t_query_master* queryMaster){
    pthread_mutex_lock(mutexLista);
    list_remove_element(lista,queryMaster);
    pthread_mutex_unlock(mutexLista);
}

//MONITOR: Ordena una lista de querys segun prioridad (mutua exclusion)
void ordenarListaQuerysMaster(pthread_mutex_t* mutexLista,t_list* listaQuery){
    pthread_mutex_lock(mutexLista);
    list_sort (listaQuery, mayorPrioridad);
    pthread_mutex_unlock(mutexLista);
}

//MONITOR: Obtiene una query de una lista (mutua exclusion)
t_query_master* queryMasterListaSegunPosicion(pthread_mutex_t* mutexLista,t_list* listaQuery,int posicion){
    t_query_master* queryMaster;
    pthread_mutex_lock(mutexLista);
    queryMaster = (t_query_master*) list_get(listaQuery,posicion);	
    pthread_mutex_unlock(mutexLista);
    return queryMaster;
}

bool queryControlDesconectado(t_query_master* queryMaster){
    pthread_mutex_lock(&queryMaster->mutexDesconexionQueryControl);
    bool desconexion = queryMaster->desconexionQueryControl;
    pthread_mutex_unlock(&queryMaster->mutexDesconexionQueryControl);
    return desconexion;
}

void desconectarQueryControl(t_query_master* queryMaster){
    pthread_mutex_lock(&queryMaster->mutexDesconexionQueryControl);
    queryMaster->desconexionQueryControl = true;
    pthread_mutex_unlock(&queryMaster->mutexDesconexionQueryControl);
}

t_query_master* obtenerQueryMasterDeReadyPorId(int idQuery){
    bool mismoId(void* queryVoid) {
        t_query_master* queryMaster = (t_query_master*) queryVoid;
        return queryMaster->query->id == idQuery;
    }
    pthread_mutex_lock(&mutexListaReady);
    t_query_master* queryMaster = (t_query_master*)list_find(listaReady,mismoId);
    pthread_mutex_unlock(&mutexListaReady);
    return queryMaster;
}

bool todosLosWorkersTienenQuery() {
    bool workerEjecutandoQuery(void* workerVoid) {
        t_worker* worker = (t_worker*) workerVoid;
        return worker->idQuery >= 0;
    }
    pthread_mutex_lock(&mutexListaWorker);
    bool todosEstanEjecutando = list_all_satisfy(listaWorker,workerEjecutandoQuery);
    pthread_mutex_unlock(&mutexListaWorker);
    return todosEstanEjecutando;
}