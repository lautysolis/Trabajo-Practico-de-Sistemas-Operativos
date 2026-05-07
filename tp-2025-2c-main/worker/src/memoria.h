#ifndef MEMORIA_H_
#define MEMORIA_H_

#include <utils/paquetes.h>
#include <utils/cliente.h>
#include <utils/utils.h>
#include <utils/serializacion.h>
#include <utils/estructuras.h>
#include <worker.h>

t_bitarray *crearBitmapWorker();
int asignarMarco(char *, int, int);
t_pagina *crearPagina(int, int, int, int);
t_tablaDePaginas *crearTabla(char *, int, int, int);

t_respuesta_storage *escribirMemoria(t_instruccion *);
t_respuesta_storage *leerMemoria(t_instruccion *, char**);
t_respuesta_storage *persistirMemoria(char*,int);
t_respuesta_storage *persistirMemoriaCompleta(int);

t_respuesta_storage *realizarEscrituraDeBloque(char *, int , int , int);
t_respuesta_storage *realizarLecturaDeBloque(char *, int , int);

void escribirContenidoEnMarco(void **, int*, int , int );
char *leerContenidoParaMaster(int *, int, int);
char *leerContenidoParaStorage(int *, int, int);

void agregarPagATabla(t_tablaDePaginas *, int, int, int, int);
void agregarPaginasAusentes(t_tablaDePaginas *, int, int);

t_list *tomarAPartirDe(int , t_list *);

int liberarMarco(char *, int, int);
t_paginaGlobal *elegirVictimaLRU();
t_paginaGlobal *elegirVictimaClockM();
void aumentarPunteroClock();
void liberarVictima(t_paginaGlobal *,int);
void agregarATablaGlobal(t_pagina *, char *);
void agregarClockM(t_paginaGlobal *);
void agregarLRU(t_paginaGlobal *);
t_paginaGlobal *crearPaginaGlobal(t_pagina *, char *);
t_tablaDePaginas *encontrarTablaConFileTag(char *);
int encontrarNumeroDePagina(t_tablaDePaginas *, t_pagina *);

bool estaPresente(void *);
bool noTienePagsPresentes(t_tablaDePaginas *);


char *codigoAString(codigo_instruccion);


#endif