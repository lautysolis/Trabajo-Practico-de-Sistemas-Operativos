#ifndef UTILS_SERVIDOR_H_
#define UTILS_SERVIDOR_H_

#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>
#include<string.h>
#include<commons/log.h>
#include<commons/collections/list.h>
#include<assert.h>
#include<commons/string.h>
#include<commons/config.h>
#include<readline/readline.h>
#include <pthread.h>
#include <semaphore.h>

extern t_log* logger;

int iniciarServidor(char*);
void esperarCliente(int,void*);
void handshakeServidor(int);
void handshakeStorage(int,int);

#endif