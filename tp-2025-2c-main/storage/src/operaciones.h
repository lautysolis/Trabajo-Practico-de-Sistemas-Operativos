#ifndef OPERACIONES_H_
#define OPERACIONES_H_

#include <storage.h>
#include <metadata.h>
#include <storage.h>

extern superblock_config* superblock;

void createFile(t_instruccion*, int);
void truncateFile(t_instruccion*, int);
void tagFile(t_instruccion*, int);
void commitFile(t_instruccion*, int);
void verificarWriteFile(t_instruccion *, int);
void verificarReadFile(t_instruccion *, int);
void writeBlock(t_instruccion*, int);
void readBlock(t_instruccion*, int);
void deleteTag(t_instruccion*, int);
bool verificarFile(char*);
bool verificarTag(char*, char*);
bool existeDirectorio(char*);
int verificarDentroFileTag(t_instruccion*);
int verificarOperacionesBase(t_instruccion*, int);
void bloqueDestroy(void*);

#endif