#ifndef EJECUCION_H_
#define EJECUCION_H_

#include <utils/paquetes.h>
#include <utils/cliente.h>
#include <utils/utils.h>
#include <utils/serializacion.h>
#include <utils/estructuras.h>
#include <worker.h>
#include <math.h>
#include <commons/temporal.h>


void escucharMaster();
void *queryInterpreter();
int huboInterrupcion();
char *leerLineaPorIndice(FILE *, int);
t_respuesta_storage *ejecutarInstruccion(t_instruccion *, int32_t);
t_instruccion *interpretarInstruccion(char *, int);
char *crearPathFinal(char *);
codigo_instruccion obtenerCodigoOperacion(char *);


#endif