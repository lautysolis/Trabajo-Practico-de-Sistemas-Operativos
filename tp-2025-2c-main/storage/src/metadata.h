#ifndef METADATA_H_
#define METADATA_H_

#include <storage.h>
#include <paths.h>
#include <utils/estructuras.h>


void crear_meta(char*, char*, int32_t);
int cantidad_bloques_meta(t_instruccion*);
t_list* obtener_bloques_meta(t_instruccion*);
void cambiar_tamanio_meta(char*, char*, int);
int pasar_estado_commited_meta(char*, char*);
void modificar_bloques_meta(char*, char*, t_list*);
int verificarEstadoCommited(char*, char*);

#endif 