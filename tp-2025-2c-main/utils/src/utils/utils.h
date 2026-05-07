#ifndef UTILS_UTILS_H_
#define UTILS_UTILS_H_

#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>
#include<string.h>
#include<commons/log.h>
#include<commons/collections/list.h>
#include <commons/collections/queue.h>
#include<assert.h>
#include<commons/string.h>
#include<commons/config.h>
#include<readline/readline.h>
#include <pthread.h>
#include <semaphore.h>
#include <commons/bitarray.h>

extern t_log* logger;
t_config* iniciar_config(char*);
int validarEntero(char*,char*);
bool stringToBool(char*);
int* intdup(int);
t_list* parsear_lista_enteros(char*);
char* lista_enteros_a_string(t_list*);

#endif
