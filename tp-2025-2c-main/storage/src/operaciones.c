#include <operaciones.h>

void createFile(t_instruccion *instruccion, int socketConWorker)
{
    pthread_mutex_t* mutexFileTag = obtenerMutexFileTag(instruccion->nombreFile, instruccion->tag);
    pthread_mutex_lock(mutexFileTag);
    if(!verificarOperacionesBase(instruccion,socketConWorker)){
        pthread_mutex_unlock(mutexFileTag);
        return;
    }

    char *path_file = path_file_dir(instruccion->nombreFile);
    mkdir(path_file, 0755);

    char *dir_tag = path_tag_dir(instruccion->nombreFile, instruccion->tag);
    mkdir(dir_tag, 0755);

    char *logical_blocks_path = path_tag_logical_dir(instruccion->nombreFile, instruccion->tag);
    mkdir(logical_blocks_path, 0755);

    crear_meta(instruccion->nombreFile, instruccion->tag, 0);

    log_info(logger, "##%d - File Creado %s:%s", instruccion->queryID, instruccion->nombreFile, instruccion->tag);
    free(path_file);
    free(dir_tag);
    free(logical_blocks_path);
    enviarRespuestaAWorker(socketConWorker, CORRECTO, NULL,""); 
    pthread_mutex_unlock(mutexFileTag);
}

void truncateFile(t_instruccion *instruccion, int socketConWorker)
{
    pthread_mutex_t* mutexFileTag = obtenerMutexFileTag(instruccion->nombreFile, instruccion->tag);
    pthread_mutex_lock(mutexFileTag);
    if (!verificarOperacionesBase(instruccion, socketConWorker)){
        pthread_mutex_unlock(mutexFileTag);
        return;
    }
    t_list* bloques = obtener_bloques_meta(instruccion);
    int cantidad_actual = list_size(bloques);
    int cantidad_bloques_nuevos = instruccion->tamanio / superblock->tam_bloque;

    if (cantidad_actual < cantidad_bloques_nuevos)
    {
        for (int i = cantidad_actual; i < cantidad_bloques_nuevos; i++)
        {
            agregarHardLink(0,instruccion->nombreFile,instruccion->tag,i,instruccion->queryID);
            list_add(bloques,intdup(0));
        }
    }
    else
    {
        for (int i = cantidad_actual - 1; i > cantidad_bloques_nuevos-1; i--)
        {
            int* bloque = list_remove(bloques,i);
            eliminarHardLink(*bloque, instruccion->nombreFile, instruccion->tag, i, instruccion->queryID);
            free(bloque);
        }
    }
    cambiar_tamanio_meta(instruccion->nombreFile, instruccion->tag, instruccion->tamanio);
    modificar_bloques_meta(instruccion->nombreFile,instruccion->tag,bloques);
    list_destroy_and_destroy_elements(bloques, bloqueDestroy);

    log_info(logger, "##%d - File Truncado %s:%s - Tamano: %d", instruccion->queryID, instruccion->nombreFile, instruccion->tag, instruccion->tamanio);
    enviarRespuestaAWorker(socketConWorker,CORRECTO, NULL,""); 
    pthread_mutex_unlock(mutexFileTag);
}

void tagFile(t_instruccion *instruccion, int socketConWorker)
{
    pthread_mutex_t* mutexFileTag = obtenerMutexFileTag(instruccion->nombreFile, instruccion->tag);
    pthread_mutex_lock(mutexFileTag);
    if(!verificarOperacionesBase(instruccion,socketConWorker)){
        pthread_mutex_unlock(mutexFileTag);
        return;
    }

    t_list* bloques = obtener_bloques_meta(instruccion);
    int cantidad_bloques = list_size(bloques);

    char *dir_tag = path_tag_dir(instruccion->nombreFileDestino, instruccion->tagDestino);
    mkdir(dir_tag, 0755);

    char *logical_blocks_path = path_tag_logical_dir(instruccion->nombreFileDestino, instruccion->tagDestino);
    mkdir(logical_blocks_path, 0755);

    crear_meta(instruccion->nombreFileDestino, instruccion->tagDestino, cantidad_bloques * superblock->tam_bloque);

    for (int i = 0; i < cantidad_bloques; i++)
    {
        int* bloque = list_get(bloques,i);
        agregarHardLink(*bloque,instruccion->nombreFileDestino,instruccion->tagDestino,i,instruccion->queryID);
    }

    modificar_bloques_meta(instruccion->nombreFileDestino, instruccion->tagDestino, bloques);
    
    log_info(logger, "##%d - Tag creado %s:%s", instruccion->queryID, instruccion->nombreFileDestino, instruccion->tagDestino);
    free(dir_tag);
    free(logical_blocks_path);
    list_destroy_and_destroy_elements(bloques, bloqueDestroy);
    enviarRespuestaAWorker(socketConWorker, CORRECTO, NULL,"");
    pthread_mutex_unlock(mutexFileTag);
}

void commitFile(t_instruccion *instruccion, int socketConWorker)
{
    pthread_mutex_t* mutexFileTag = obtenerMutexFileTag(instruccion->nombreFile, instruccion->tag);
    pthread_mutex_lock(mutexFileTag);
    if(!verificarOperacionesBase(instruccion,socketConWorker)){
        pthread_mutex_unlock(mutexFileTag);
        return;
    }
    
    if (!pasar_estado_commited_meta(instruccion->nombreFile, instruccion->tag)){ 
        log_info(logger,"Tag %s de File %s ya fue commiteado", instruccion->tag, instruccion->nombreFile);
        enviarRespuestaAWorker(socketConWorker, CORRECTO, NULL,"");
        pthread_mutex_unlock(mutexFileTag);
        return;
    }
    
    t_list* bloques = obtener_bloques_meta(instruccion);
    int cantidad_bloques = list_size(bloques);

    for (int i = 0; i < cantidad_bloques; i++)
    {
        int* bloqueFisico = list_get(bloques,i);
        
        int bloqueDuplicado = buscarBloquePorHash(*bloqueFisico);
        if (bloqueDuplicado == -1)
        {
            guardarHashBloque(*bloqueFisico);     
        }
        else if(bloqueDuplicado != *bloqueFisico){
            deduplicacionBloque(*bloqueFisico,bloqueDuplicado,instruccion->nombreFile,instruccion->tag,i,instruccion->queryID);
            list_replace(bloques, i, intdup(bloqueDuplicado));
            free(bloqueFisico);
        }
    }

    modificar_bloques_meta(instruccion->nombreFile, instruccion->tag,bloques);
    log_info(logger, "##%d - Commit de File:Tag %s:%s", instruccion->queryID, instruccion->nombreFile, instruccion->tag);
    list_destroy_and_destroy_elements(bloques, bloqueDestroy);
    enviarRespuestaAWorker(socketConWorker, CORRECTO, NULL,"");
    pthread_mutex_unlock(mutexFileTag);
}

void writeBlock(t_instruccion *instruccion, int socketConWorker)
{       
    pthread_mutex_t* mutexFileTag = obtenerMutexFileTag(instruccion->nombreFile, instruccion->tag);
    pthread_mutex_lock(mutexFileTag);
    if(!verificarOperacionesBase(instruccion, socketConWorker)){
        pthread_mutex_unlock(mutexFileTag);
        return;
    }

    t_list* bloques = obtener_bloques_meta(instruccion);

    int* bloqueFisico = list_get(bloques, instruccion->nroBloque);
    char* dirBloqueFisico = path_physical_block(*bloqueFisico);
    struct stat info;
    retardo_bloque();
    if (stat(dirBloqueFisico, &info)==0 && info.st_nlink == 2){
        escribirBloque(instruccion, *bloqueFisico);
    }
    else{
        int bloqueLibre = buscarBloqueLibre();
        if(bloqueLibre == -1){
            list_destroy_and_destroy_elements(bloques, bloqueDestroy);
            free(dirBloqueFisico);
            char* error = string_from_format("ERROR AL ESCRIBIR BLOQUE: Espacio insuficiente para escribir sobre file:tag %s:%s", instruccion->nombreFile, instruccion->tag); 
            log_error(logger, "%s", error);
            enviarRespuestaAWorker(socketConWorker, ERROR_INSUFICIENTE, NULL,error);
            free(error);
            pthread_mutex_unlock(mutexFileTag);
            return;
        }
        escribirBloque(instruccion,bloqueLibre);
        eliminarHardLink(*bloqueFisico, instruccion->nombreFile, instruccion->tag, instruccion->nroBloque, instruccion->queryID);
        agregarHardLink(bloqueLibre, instruccion->nombreFile, instruccion->tag, instruccion->nroBloque, instruccion->queryID);
        list_replace(bloques, instruccion->nroBloque, intdup(bloqueLibre));
        modificar_bloques_meta(instruccion->nombreFile,instruccion->tag,bloques);
        free(bloqueFisico);
        marcarBloqueOcupado(bloqueLibre,instruccion->queryID);
    }
    log_info(logger, "##%d - Bloque Logico Escrito %s:%s - Numero de Bloque: %06d", instruccion->queryID,instruccion->nombreFile,instruccion->tag,instruccion->nroBloque);
    list_destroy_and_destroy_elements(bloques, bloqueDestroy);
    free(dirBloqueFisico);

    enviarRespuestaAWorker(socketConWorker, CORRECTO, NULL , "");
    pthread_mutex_unlock(mutexFileTag);
}

void readBlock(t_instruccion *instruccion, int socketConWorker)
{
    pthread_mutex_t* mutexFileTag = obtenerMutexFileTag(instruccion->nombreFile, instruccion->tag);
    pthread_mutex_lock(mutexFileTag);
    if(!verificarOperacionesBase(instruccion,socketConWorker)){
        pthread_mutex_unlock(mutexFileTag);
        return;
    }
    
    t_list* bloques = obtener_bloques_meta(instruccion);
    
    int* bloqueFisico = list_get(bloques, instruccion->nroBloque);

    retardo_bloque();

    void* contenido = leerBloque(*bloqueFisico);

    log_info(logger, "##%d - Bloque Logico Leido %s:%s - Numero de Bloque: %06d", instruccion->queryID, instruccion->nombreFile, instruccion->tag, instruccion->nroBloque);
    
    list_destroy_and_destroy_elements(bloques, bloqueDestroy);
    enviarRespuestaAWorker(socketConWorker, CORRECTO, contenido,"");
    free(contenido);
    pthread_mutex_unlock(mutexFileTag);
}

void deleteTag(t_instruccion *instruccion, int socketConWorker){

    pthread_mutex_t* mutexFileTag = obtenerMutexFileTag(instruccion->nombreFile, instruccion->tag);
    pthread_mutex_lock(mutexFileTag);
    if(!verificarOperacionesBase(instruccion, socketConWorker)) {
        pthread_mutex_unlock(mutexFileTag);
        return;
    }

    t_list *bloques = obtener_bloques_meta(instruccion);
    int cantidad_bloques = list_size(bloques);

    for (int i = 0; i < cantidad_bloques; i++)
    {
        int *bloqueFisico = list_get(bloques, i);
        eliminarHardLink(*bloqueFisico, instruccion->nombreFile, instruccion->tag, i, instruccion->queryID);
    }
    char* dirMetadata = path_tag_metadata(instruccion->nombreFile, instruccion->tag);
    char* dirLogicalBlocks = path_tag_logical_dir(instruccion->nombreFile, instruccion->tag);
    char* dirTag = path_tag_dir(instruccion->nombreFile, instruccion->tag);
    remove(dirMetadata);
    rmdir(dirLogicalBlocks);
    rmdir(dirTag);

    log_info(logger, "##%d - Tag Eliminado %s:%s", instruccion->queryID, instruccion->nombreFile, instruccion->tag);
    list_destroy_and_destroy_elements(bloques, bloqueDestroy);
    free(dirMetadata);
    free(dirLogicalBlocks);
    free(dirTag);
    enviarRespuestaAWorker(socketConWorker, CORRECTO, NULL,"");
    pthread_mutex_unlock(mutexFileTag);
}

bool verificarFile(char* nombreFile){
    char *file = path_file_dir(nombreFile);
    bool existe = existeDirectorio(file);
    free(file);

    return existe;
}

bool verificarTag(char* nombreFile, char* tag) {
    char *file = path_tag_dir(nombreFile,tag);
    bool existe = existeDirectorio(file);
    free(file);

    return existe; 
}

bool existeDirectorio(char* path) {
    struct stat st;

    if (stat(path, &st) == 0) {
        return S_ISDIR(st.st_mode);
    }

    return false;
}

int verificarDentroFileTag(t_instruccion* instruccion){
    t_list* bloques = obtener_bloques_meta(instruccion);
    int size = list_size(bloques);
    int dentro = (instruccion->nroBloque >= 0 && instruccion->nroBloque < size);
    list_destroy_and_destroy_elements(bloques, bloqueDestroy);

    return dentro;   
}

int verificarOperacionesBase(t_instruccion* instruccion, int socketConWorker) {
    
    switch(instruccion->codigoIns) {

        case CREATE:
            if (verificarFile(instruccion->nombreFile)){
                char* error = string_from_format("ERROR AL CREAR FILE: File %s preexistente", instruccion->nombreFile); 
                log_error(logger, "%s", error);
                enviarRespuestaAWorker(socketConWorker, ERROR_PREEXISTENTE, NULL, error);
                free(error);
                return 0;
            }
            return 1;

        case TRUNCATE:
            if (!verificarFile(instruccion->nombreFile)){
                char* error = string_from_format("ERROR AL TRUNCAR FILE: File %s inexistente", instruccion->nombreFile); 
                log_error(logger, "%s", error);
                enviarRespuestaAWorker(socketConWorker, ERROR_INEXISTENTE,NULL, error);
                free(error);
                return 0;
            }
            else if (!verificarTag(instruccion->nombreFile, instruccion->tag)) {
                char* error = string_from_format("ERROR AL TRUNCAR FILE: Tag %s de File %s inexistente", instruccion->tag, instruccion->nombreFile); 
                log_error(logger, "%s", error);
                enviarRespuestaAWorker(socketConWorker, ERROR_INEXISTENTE,NULL, error);
                free(error);
                return 0;
            }
            else if (verificarEstadoCommited(instruccion->nombreFile, instruccion->tag)){ 
                char* error = string_from_format("ERROR AL TRUNCAR FILE: Operacion prohibida sobre file:tag %s:%s que se encuentra comiteado", instruccion->nombreFile, instruccion->tag); 
                log_error(logger, "%s", error);
                enviarRespuestaAWorker(socketConWorker, ERROR_PROHIBIDO,NULL,error);
                free(error);
                return 0;
            }
            return 1;

        case TAG:
            if (!verificarFile(instruccion->nombreFile)){
                char* error = string_from_format("ERROR AL TAGGEAR FILE: File %s inexistente", instruccion->nombreFile); 
                log_error(logger, "%s", error);
                enviarRespuestaAWorker(socketConWorker, ERROR_INEXISTENTE,NULL, error);
                free(error);
                return 0;
            }
            else if (!verificarTag(instruccion->nombreFile, instruccion->tag)) {
                char* error = string_from_format("ERROR AL TAGGEAR FILE: Tag %s de File %s inexistente", instruccion->tag, instruccion->nombreFile); 
                log_error(logger, "%s", error);
                enviarRespuestaAWorker(socketConWorker, ERROR_INEXISTENTE,NULL, error);
                free(error);
                return 0;
            }
            else if (!verificarFile(instruccion->nombreFileDestino)){
                char* error = string_from_format("ERROR AL TAGGEAR FILE: File %s destino inexistente", instruccion->nombreFileDestino); 
                log_error(logger, "%s", error);
                enviarRespuestaAWorker(socketConWorker, ERROR_INEXISTENTE,NULL, error);
                free(error);
                return 0;
            }
            else if (verificarTag(instruccion->nombreFileDestino, instruccion->tagDestino)){
                char* error = string_from_format("ERROR AL TAGGEAR FILE: Tag %s de File %s preexistente", instruccion->tagDestino, instruccion->nombreFileDestino); 
                log_error(logger, "%s", error);
                enviarRespuestaAWorker(socketConWorker, ERROR_PREEXISTENTE,NULL,error);
                free(error);
                return 0;
            }
            return 1;

        case COMMIT:
            if (!verificarFile(instruccion->nombreFile)){
                char* error = string_from_format("ERROR AL COMMITEAR FILE: File %s inexistente", instruccion->nombreFile); 
                log_error(logger, "%s", error);
                enviarRespuestaAWorker(socketConWorker, ERROR_INEXISTENTE,NULL, error);
                free(error);
                return 0;
            }
            else if (!verificarTag(instruccion->nombreFile, instruccion->tag)){
                char* error = string_from_format("ERROR AL COMMITEAR FILE: Tag %s de File %s inexistente", instruccion->tag, instruccion->nombreFile); 
                log_error(logger, "%s", error);
                enviarRespuestaAWorker(socketConWorker, ERROR_INEXISTENTE,NULL,error);
                free(error);
                return 0;
            }

            return 1;

        case WRITE_BLOCK:
            if (!verificarFile(instruccion->nombreFile)){
                char* error = string_from_format("ERROR AL ESCRIBIR BLOQUE: File %s inexistente", instruccion->nombreFile); 
                log_error(logger, "%s", error);
                enviarRespuestaAWorker(socketConWorker, ERROR_INEXISTENTE,NULL, error);
                free(error);
                return 0;
            }
            else if(!verificarTag(instruccion->nombreFile, instruccion->tag)) {
                char* error = string_from_format("ERROR AL ESCRIBIR BLOQUE: Tag %s de File %s inexistente", instruccion->tag, instruccion->nombreFile); 
                log_error(logger, "%s", error);
                enviarRespuestaAWorker(socketConWorker, ERROR_INEXISTENTE,NULL, error);
                free(error);
                return 0;
            }
            else if (verificarEstadoCommited(instruccion->nombreFile, instruccion->tag)){ 
                char* error = string_from_format("ERROR AL ESCRIBIR BLOQUE: Operacion prohibida sobre file:tag %s:%s que se encuentra comiteado", instruccion->nombreFile, instruccion->tag); 
                log_error(logger, "%s", error);
                enviarRespuestaAWorker(socketConWorker, ERROR_PROHIBIDO,NULL, error);
                free(error);
                return 0;
            }
            else if(!verificarDentroFileTag(instruccion)){
                char* error = string_from_format("ERROR AL ESCRIBIR BLOQUE: Se intento escribrir por fuera del size del file:tag %s:%s", instruccion->nombreFile, instruccion->tag); 
                log_error(logger, "%s", error);
                enviarRespuestaAWorker(socketConWorker, ERROR_LIMITE,NULL, error);
                free(error);
                return 0;
            }

            return 1;

        case READ_BLOCK:
            if (!verificarFile(instruccion->nombreFile)){
                char* error = string_from_format("ERROR AL LEER BLOQUE: File %s inexistente", instruccion->nombreFile); 
                log_error(logger, "%s", error);
                enviarRespuestaAWorker(socketConWorker, ERROR_INEXISTENTE,NULL, error);
                free(error);
                return 0;
            }
            else if(!verificarTag(instruccion->nombreFile, instruccion->tag)) {
                char* error = string_from_format("ERROR AL LEER BLOQUE: Tag %s de File %s inexistente", instruccion->tag, instruccion->nombreFile); 
                log_error(logger, "%s", error);
                enviarRespuestaAWorker(socketConWorker, ERROR_INEXISTENTE,NULL, error);
                free(error);
                return 0;
            }
            else if(!verificarDentroFileTag(instruccion)){
                char* error = string_from_format("ERROR AL LEER BLOQUE: Se intento leer por fuera del size del file:tag %s:%s", instruccion->nombreFile, instruccion->tag); 
                log_error(logger, "%s", error);
                enviarRespuestaAWorker(socketConWorker,ERROR_LIMITE,NULL,error);
                free(error);
                return 0;
            }

            return 1;

        case DELETE:
            if (!verificarFile(instruccion->nombreFile)){
                char* error = string_from_format("ERROR AL ELIMINAR TAG: File %s inexistente", instruccion->nombreFile); 
                log_error(logger, "%s", error);
                enviarRespuestaAWorker(socketConWorker, ERROR_INEXISTENTE,NULL, error);
                free(error);
                return 0;
            }
            else if(!verificarTag(instruccion->nombreFile, instruccion->tag)) {
                char* error = string_from_format("ERROR AL ELIMINAR TAG: Tag %s de File %s inexistente", instruccion->tag, instruccion->nombreFile); 
                log_error(logger, "%s", error);
                enviarRespuestaAWorker(socketConWorker, ERROR_INEXISTENTE,NULL, error);
                free(error);
                return 0;
            }
            return 1;

        default:
            return 0;
    }
}

void bloqueDestroy(void* ptr){
    int* bloque = (int*) ptr;
    free(bloque);
}