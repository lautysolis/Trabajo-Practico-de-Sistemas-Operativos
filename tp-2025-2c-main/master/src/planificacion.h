#ifndef PLANIFICACION_H_
#define PLANIFICACION_H_

#include <utils/utils.h>
#include <utils/serializacion.h>
#include <utils/estructuras.h>
#include <utils/servidor.h>
#include <utils/paquetes.h>
#include <unistd.h>
#include <master.h>

extern t_log* logger;
extern master_config* configuracion;

void iniciarPlanificacion(void);
void atenderQuery(t_paquete*,int);
void* ready(void*);
void* exitStatus(void*);
void atenderWorker(t_paquete*,int);
void comenzarAging(t_query_master*);
void* aging(void* queryVoid);
void desalojarQuery(t_query_master*,t_respuesta_query*,int);
void verificarDesalojo(t_query_master*);
t_query_master* obtenerSiguienteQueryMasterReady();
void asignarQueryId(t_query_master* query);
int obtenerGradoMultiprocesamiento(void);
int obtenerPrioridadQuery(t_query_master*);
void disminuirPrioridadQuery(t_query_master*);
void cambiarCodAccionQuery(t_query_master*, codigo_accion_query);
bool siempreTrue(void*);
void agregarWorkerLista(t_worker*);
void eliminarWorkerLista(t_worker*);
void agregarQueryMasterCola(pthread_mutex_t*,t_queue*,t_query_master*);
t_query_master* sacarQueryMasterCola(pthread_mutex_t*,t_queue*);
void agregarQueryMasterLista(pthread_mutex_t*,t_list*,t_query_master*);
bool mayorPrioridad(void*, void*);
t_query_master* sacarQueryMasterLista(pthread_mutex_t*,t_list*,int);
void sacarQueryMasterListaPasadoPorParametro(pthread_mutex_t*,t_list*,t_query_master*);
void ordenarListaQuerysMaster(pthread_mutex_t*,t_list*);
t_query_master* queryMasterListaSegunPosicion(pthread_mutex_t*,t_list*,int);
void enviarDesalojo(t_query_master*);
int lengthLista(pthread_mutex_t*,t_list*);
void manejarDesconexionWorker(t_query_master*,t_worker*);
void desconectarQueryControl(t_query_master*);
bool queryControlDesconectado(t_query_master*);
t_worker* obtenerWorkerEjecutandoQuery(int);
void enviarExit(t_query_master*,t_respuesta_query*,int,char*);
void manejarDesconexionQueryControl(t_query_master*,int);
t_query_master* obtenerQueryMasterDeReadyPorId(int);
codigo_accion_query obtenerCodAccion(t_query_master*);
bool todosLosWorkersTienenQuery();

#endif
