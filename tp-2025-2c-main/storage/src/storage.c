#include <storage.h>
#include <operaciones.h>
t_paths_fs *fs_paths;
t_bitarray *bitmap;
int cantidadWorkers;
pthread_mutex_t cantidadWorkers_mutex;
pthread_mutex_t bitmap_mutex;
pthread_mutex_t hashIndex_mutex;
extern storage_config *configuracion;
extern superblock_config *superblock;
pthread_mutex_t diccionarioFileTag_mutex;
t_dictionary* diccionarioFileTag;

void iniciarStorage(char *argv[])
{
    char *archivoConfig = argv[1];

    crearStorageConfig(archivoConfig);

    logger = log_create("storage.log", "Storage", true, configuracion->nivelLog);

    inicializarPaths();

    iniciarSuperblock();

    pthread_mutex_init(&cantidadWorkers_mutex,NULL);
    pthread_mutex_init(&hashIndex_mutex,NULL);
    pthread_mutex_init(&bitmap_mutex, NULL);
    pthread_mutex_init(&diccionarioFileTag_mutex, NULL);
    diccionarioFileTag = dictionary_create();
}

void inicializarPaths() {
    fs_paths = malloc(sizeof(t_paths_fs));

    fs_paths->path_superblock = string_from_format("%s/superblock.config", configuracion->puntoMontaje);
    fs_paths->path_bitmap = string_from_format("%s/bitmap.bin", configuracion->puntoMontaje);
    fs_paths->path_blocks_hash_index = string_from_format("%s/blocks_hash_index.config", configuracion->puntoMontaje);
    fs_paths->path_physicalblocks = string_from_format("%s/physical_blocks", configuracion->puntoMontaje);
    fs_paths->path_files = string_from_format("%s/files", configuracion->puntoMontaje);
    fs_paths->initialfile = string_from_format("%s/files/initial_file", configuracion->puntoMontaje);
}


void crearStorageConfig(char *archivoConfig)
{   
    t_config *config = iniciar_config(archivoConfig);

    char *puerto = config_get_string_value(config, "PUERTO_STORAGE");
    char *freshStartString = config_get_string_value(config, "FRESH_START");
    char *puntoMontaje = config_get_string_value(config, "PUNTO_MONTAJE");
    int retardoOperacion = config_get_int_value(config, "RETARDO_OPERACION");
    int retardoAccesoBloque = config_get_int_value(config, "RETARDO_ACCESO_BLOQUE");
    char *nivelString = config_get_string_value(config, "LOG_LEVEL");
    t_log_level nivelLog = log_level_from_string(nivelString);
    bool freshStart = stringToBool(freshStartString);

    configuracion = malloc(sizeof(storage_config));

    configuracion->puntoMontaje = malloc(strlen(puntoMontaje) + 1);
    memcpy(configuracion->puntoMontaje, puntoMontaje, strlen(puntoMontaje) + 1);
    configuracion->puerto = malloc(strlen(puerto) + 1);
    memcpy(configuracion->puerto, puerto, strlen(puerto) + 1);

    configuracion->retardoOperacion = retardoOperacion;
    configuracion->retardoAccesoBloque = retardoAccesoBloque;
    configuracion->nivelLog = nivelLog;
    configuracion->freshStart = freshStart;

    config_destroy(config);
}

void iniciarSuperblock()
{
    superblock = malloc(sizeof(superblock_config));

    t_config *config = config_create(fs_paths->path_superblock);

    superblock->tam_bloque = config_get_int_value(config, "BLOCK_SIZE");
    superblock->tam_fs = config_get_int_value(config, "FS_SIZE");
    superblock->cantidad_bloques = superblock->tam_fs/superblock->tam_bloque;

    config_destroy(config);
}

pthread_mutex_t* obtenerMutexFileTag(char* file, char* tag) {
    char* fileTag = string_from_format("%s:%s", file, tag);
    pthread_mutex_lock(&diccionarioFileTag_mutex);
    pthread_mutex_t* mutex = dictionary_get(diccionarioFileTag, fileTag);

    if(!mutex) {
        mutex = malloc(sizeof(pthread_mutex_t));
        pthread_mutex_init(mutex, NULL);
        dictionary_put(diccionarioFileTag, fileTag, mutex);
    }

    pthread_mutex_unlock(&diccionarioFileTag_mutex);
    free(fileTag);
    return mutex;
}

void iniciarFileSystem()
{
    if (configuracion->freshStart)
    {   
        log_info(logger, "Formateando nuevo File System en: %s", configuracion->puntoMontaje);
        

        crearBitmap();
        crearHashIndex();
        crearPhysicalBlocks();
        crearEstructuraFile();
    }
    else
    {
        log_info(logger, "Se reutiliza File System existente en: %s", configuracion->puntoMontaje);
        cargarBitmapExistente();
    }
}

void crearBitmap()
{
    remove(fs_paths->path_bitmap);
    FILE *bitmapFile = fopen(fs_paths->path_bitmap, "wb+");
    
    if (!bitmapFile)
    {
        log_error(logger, "Error al crear bitmap.bin");
        return;
    }

    crearBitArray(bitmapFile);
    
    for (int bloque = 0; bloque < bitarray_get_max_bit(bitmap); bloque++)
    {
        bitarray_clean_bit(bitmap, bloque);
    }
    fclose(bitmapFile);
    msync(bitmap->bitarray, bitmap->size, MS_SYNC);
}

void crearBitArray(FILE* bitmapFile){
    
    int bytes_bitmap = (superblock->cantidad_bloques + 7) / 8;
    int fd = fileno(bitmapFile);
    ftruncate(fd,bytes_bitmap);

    void* bitmap_mem = mmap(0, bytes_bitmap, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    bitmap = bitarray_create_with_mode(bitmap_mem, bytes_bitmap, LSB_FIRST);
}

void crearHashIndex()
{
    remove(fs_paths->path_blocks_hash_index);
    FILE *indexFile = fopen(fs_paths->path_blocks_hash_index, "wb");

    if (!indexFile)
    {
        log_error(logger, "Error al crear hash_index");
        return;
    }

    fclose(indexFile);
}

void crearPhysicalBlocks()
{
    char* comandoRemove = string_from_format("rm -rf -- %s",fs_paths->path_physicalblocks);
    system(comandoRemove);
    free(comandoRemove);
    mkdir(fs_paths->path_physicalblocks, 0755);

    for (int i = 0; i < superblock->cantidad_bloques; i++)
    {
        char *fileBlock = string_from_format("%s/block%04d.dat", fs_paths->path_physicalblocks, i);
        FILE *bloqueFile = fopen(fileBlock, "wb");
        if (!bloqueFile)
        {
            log_error(logger, "Error al crear bloque fisico %d", i);
            free(fileBlock);
            continue;
        }
        int fd = fileno(bloqueFile);
        ftruncate(fd, superblock->tam_bloque);
        fclose(bloqueFile);
        free(fileBlock);
    }
}

void crearEstructuraFile()
{
    char* comandoRemove = string_from_format("rm -rf -- %s",fs_paths->path_files);
    system(comandoRemove);
    free(comandoRemove);
    
    mkdir(fs_paths->path_files, 0755);
    crearArchivoInicial();
}

void crearArchivoInicial()
{
    mkdir(fs_paths->initialfile, 0755);

    char *basePath = path_tag_dir("initial_file","BASE");
    mkdir(basePath, 0755);

    char *metadataInitial = path_tag_metadata("initial_file","BASE");
    FILE *metaFile = fopen(metadataInitial, "wb");

    if (!metaFile)
    {
        log_error(logger, "No se pudo crear %s", metadataInitial);
        free(basePath);
        free(metadataInitial);
        return;
    }
    fclose(metaFile);

    t_config *configMeta = iniciar_config(metadataInitial);
    char* tamanio = string_itoa(superblock->tam_bloque);
    config_set_value(configMeta, "TAMAÑO", tamanio);
    config_set_value(configMeta, "BLOCKS", "[0]");
    config_set_value(configMeta, "ESTADO", "COMMITED");
    config_save(configMeta);
    config_destroy(configMeta);

    char *logicalBlockInitial = path_tag_logical_dir("initial_file", "BASE");
    mkdir(logicalBlockInitial, 0755);

    char* fisico0 = path_physical_block(0);
    char* logico0 = path_logical_block("initial_file", "BASE", 0);

    link(fisico0,logico0);
    
    marcarBloqueOcupadoBitArray(0);
    guardarHashBloque(0);

    free(basePath);
    free(metadataInitial);
    free(tamanio);
    free(logicalBlockInitial);
    free(fisico0);
    free(logico0);
}

void cargarBitmapExistente() {

    FILE* bitmapFile = fopen(fs_paths->path_bitmap, "rb+");
    if (!bitmapFile) {
        log_error(logger, "No se pudo abrir bitmap existente");
        return;
    }

    crearBitArray(bitmapFile);

    fclose(bitmapFile);

    log_info(logger, "Bitmap existente cargado correctamente");
}

int buscarBloqueLibre()
{
    pthread_mutex_lock(&bitmap_mutex);
    int bloqueLibre = -1;
    for (int i = 0; i < bitarray_get_max_bit(bitmap); i++)
    {
        if (!bitarray_test_bit(bitmap, i))
        {
            bloqueLibre = i;
            break;
        }
    }
    pthread_mutex_unlock(&bitmap_mutex);
    return bloqueLibre;
}

void agregarHardLink(int bloqueFisico, char* nombreFile, char* tag, int bloqueLogico, int queryID){
    char* dirBloqueFisico = path_physical_block(bloqueFisico);
    char* dirBloqueLogico = path_logical_block(nombreFile, tag, bloqueLogico);
    link(dirBloqueFisico,dirBloqueLogico);
    log_info(logger,"## %d - %s:%s Se agrego el hard link del bloque logico %06d al bloque fisico %04d",queryID,nombreFile,tag,bloqueLogico,bloqueFisico);
    free(dirBloqueLogico);
    free(dirBloqueFisico);
}

void eliminarHardLink(int bloqueFisico, char* nombreFile, char* tag, int bloqueLogico, int queryID){
    char* dirBloqueLogico = path_logical_block(nombreFile, tag, bloqueLogico);
    char* dirBloqueFisico = path_physical_block(bloqueFisico);
    unlink(dirBloqueLogico);
    struct stat info;
    if (stat(dirBloqueFisico, &info) == 0 && info.st_nlink == 1)
    {
        liberarBloque(bloqueFisico,queryID);
        eliminarHashBloque(bloqueFisico);
    }
    log_info(logger,"##%d - %s:%s Se elimino el hard link del bloque logico %06d al bloque fisico %04d",queryID,nombreFile,tag,bloqueLogico,bloqueFisico);
    free(dirBloqueLogico);
    free(dirBloqueFisico);
}

void deduplicacionBloque(int bloqueFisicoAnterior,int bloqueFisicoNuevo, char* nombreFile,char* tag, int bloqueLogico, int queryID){
    char* dirBloqueLogico = path_logical_block(nombreFile, tag, bloqueLogico);
    char* dirBloqueFisicoAnterior = path_physical_block(bloqueFisicoAnterior);
    char* dirBloqueFisicoNuevo = path_physical_block(bloqueFisicoNuevo);
    unlink(dirBloqueLogico);
    link(dirBloqueFisicoNuevo,dirBloqueLogico);
    log_info(logger,"##%d - %s:%s bloque logico %06d se reasigna de bloque fisico %04d al bloque fisico %04d",queryID,nombreFile,tag,bloqueLogico,bloqueFisicoAnterior,bloqueFisicoNuevo);
    struct stat info;
    if (stat(dirBloqueFisicoAnterior, &info) == 0 && info.st_nlink == 1)
    {
        liberarBloque(bloqueFisicoAnterior,queryID);
    }
    free(dirBloqueLogico);
    free(dirBloqueFisicoAnterior);
    free(dirBloqueFisicoNuevo);
}

void marcarBloqueOcupado(int bloque, int queryID){
    marcarBloqueOcupadoBitArray(bloque);
    log_info(logger, "## %d - Bloque Fisico Reservado - Numero de Bloque: %04d", queryID, bloque);

}
void marcarBloqueOcupadoBitArray(int bloque){
    pthread_mutex_lock(&bitmap_mutex);
    bitarray_set_bit(bitmap, bloque);
    msync(bitmap->bitarray, bitmap->size, MS_SYNC);
    pthread_mutex_unlock(&bitmap_mutex);
}

void liberarBloque(int bloque,int queryID)
{
    liberarBloqueBitArray(bloque);
    log_info(logger,"##%d - Bloque Fisico Liberado - Numero de Bloque: %04d", queryID, bloque);
}

void liberarBloqueBitArray(int bloque){
    pthread_mutex_lock(&bitmap_mutex);
    bitarray_clean_bit(bitmap, bloque);
    msync(bitmap->bitarray, bitmap->size, MS_SYNC);
    pthread_mutex_unlock(&bitmap_mutex);
}

char* calcularHash(int bloque)
{

    char *fileBlock = path_physical_block(bloque);

    FILE *file = fopen(fileBlock, "rb");

    t_buffer *buffer = crearBuffer(superblock->tam_bloque);
    fread(buffer->stream, 1, superblock->tam_bloque, file); 
    fclose(file);

    char *hash = crypto_md5(buffer->stream, superblock->tam_bloque);

    free(buffer->stream);
    free(buffer);
    free(fileBlock);

    return hash;
}

void guardarHashBloque(int bloque)
{
    char* hash = calcularHash(bloque);
    pthread_mutex_lock(&hashIndex_mutex);
    t_config* config = iniciar_config(fs_paths->path_blocks_hash_index);

    char* block = string_from_format("BLOCK%04d", bloque);
    config_set_value(config, hash, block);

    config_save(config);
    config_destroy(config);
    pthread_mutex_unlock(&hashIndex_mutex);

    free(block);
    free(hash);
}

void eliminarHashBloque(int bloque)
{
    char* hash = calcularHash(bloque);
    pthread_mutex_lock(&hashIndex_mutex);
    t_config* config = iniciar_config(fs_paths->path_blocks_hash_index);

    config_remove_key(config, hash);

    config_save(config);
    config_destroy(config);
    pthread_mutex_unlock(&hashIndex_mutex);

    free(hash);
}

int buscarBloquePorHash(int bloque){
    char* hash = calcularHash(bloque);

    pthread_mutex_lock(&hashIndex_mutex);

    t_config* config = iniciar_config(fs_paths->path_blocks_hash_index);

    int bloqueFisico = -1;
    if (config_has_property(config, hash))
    {
        char* bloqueFisico_str = config_get_string_value(config, hash);
        bloqueFisico = atoi(bloqueFisico_str + 5); // salteamos "BLOCK"
    }
    config_destroy(config);
    pthread_mutex_unlock(&hashIndex_mutex);
    free(hash);

    return bloqueFisico; 
}

void escribirBloque(t_instruccion* instruccion, int bloqueFisico){

    char* pathFisico = path_physical_block(bloqueFisico);

    FILE* bloque = fopen(pathFisico, "wb");

    fwrite(instruccion->contenido, 1, superblock->tam_bloque, bloque);

    fclose(bloque);
    free(pathFisico);
}

void* leerBloque(int bloqueFisico){

    char* pathFisico = path_physical_block(bloqueFisico);

    FILE* bloque = fopen(pathFisico, "rb");

    void* contenido = malloc(superblock->tam_bloque);
    
    fread(contenido, 1, superblock->tam_bloque, bloque);

    fclose(bloque);
    free(pathFisico);

    return contenido;
}

void sumarWorker(){
    pthread_mutex_lock(&cantidadWorkers_mutex);
    cantidadWorkers++;
    pthread_mutex_unlock(&cantidadWorkers_mutex);
}

void restarWorker(){
    pthread_mutex_lock(&cantidadWorkers_mutex);
    cantidadWorkers--;
    pthread_mutex_unlock(&cantidadWorkers_mutex);
}

void *atenderWorker(void *conexion)
{
    int socketConWorker = *((int *)conexion);
    free(conexion);
    int idWorker = handshakeConWorker(socketConWorker);
    sumarWorker();
    log_info(logger,"##Se conecta el Worker %d - Cantidad de Workers: %d", idWorker,cantidadWorkers);
    bool workerFuncionando = true;
    while (workerFuncionando)
    {
        t_paquete *paquete = recibirPaquete(socketConWorker);
        if (!paquete)
        {
            restarWorker();
            log_info(logger,"##Se desconecta el Worker %d - Cantidad de Workers: %d", idWorker,cantidadWorkers);
            workerFuncionando = false;
            break;
        }
        t_instruccion *instruccion = deserializarInstruccion(paquete->buffer);
        eliminarPaquete(paquete);
        retardo_operacion();
        switch (instruccion->codigoIns)
        {
        case CREATE:
            createFile(instruccion, socketConWorker);
            break;
        case TRUNCATE:
            truncateFile(instruccion, socketConWorker);
            break;
        case TAG:
            tagFile(instruccion, socketConWorker);
            break;
        case COMMIT:
            commitFile(instruccion, socketConWorker);
            break;
        case WRITE_BLOCK:
            writeBlock(instruccion,socketConWorker);
            break;
        case READ_BLOCK:
            readBlock(instruccion,socketConWorker);
            break;
        case DELETE:
            deleteTag(instruccion, socketConWorker);
            break;
        default:
            log_warning(logger, "Codigo de operacion desconocido recibido: %d", instruccion->codigoIns);
            enviarRespuestaAWorker(socketConWorker, ERROR_PROHIBIDO,NULL,"");
            break;
        }
        liberarInstruccion(instruccion);
    }
    close(socketConWorker);
    return NULL;
}

int handshakeConWorker(int socketWorker)
{
    t_paquete* paquete = recibirPaquete(socketWorker);
    int idWorker = deserializarIdWorker(paquete->buffer);
    int32_t sizeBlockEnviar = (paquete->codigoOperacion == HANDSHAKE_WORKER_STORAGE) ? superblock->tam_bloque : -1;
    enviarSizeBlock(sizeBlockEnviar, socketWorker);
    eliminarPaquete(paquete);

    return idWorker;
}

void enviarSizeBlock(int32_t sizeBlock, int socket)
{
    t_buffer *buffer = serializarSizeBlock(sizeBlock);
    enviarPaquete(socket, HANDSHAKE_WORKER_STORAGE, buffer);
}

void enviarRespuestaAWorker(int socket, codigo_respuesta_storage codigo, void* contenido,char* error){
    t_respuesta_storage* respuesta;
    if(contenido==NULL){
        contenido=malloc(superblock->tam_bloque);
        memset(contenido,0,superblock->tam_bloque);
        respuesta = crearRespuestaStorage(codigo,contenido,error,superblock->tam_bloque);
        free(contenido);
    }else{
        respuesta = crearRespuestaStorage(codigo,contenido,error,superblock->tam_bloque);
    }
    enviarRespuestaStorage(respuesta,socket);
    liberarRespuestaStorage(respuesta);
}

void finalizarStorage(int socket)
{
    log_info(logger, "Finalizando Storage");
    free(configuracion->puntoMontaje);
    free(configuracion->puerto);
    free(configuracion);
    close(socket);
    free(fs_paths->path_bitmap);
    free(fs_paths->path_blocks_hash_index);
    free(fs_paths->path_superblock);
    free(fs_paths->path_physicalblocks);
    free(fs_paths->initialfile);
    free(fs_paths->path_files);
    free(fs_paths);
    free(superblock);
    pthread_mutex_destroy(&cantidadWorkers_mutex);
    pthread_mutex_destroy(&bitmap_mutex);
    pthread_mutex_destroy(&hashIndex_mutex);
    pthread_mutex_destroy(&diccionarioFileTag_mutex);
    dictionary_destroy_and_destroy_elements (diccionarioFileTag, mutexDestroy);
    munmap(bitmap->bitarray, bitmap->size);
    bitarray_destroy(bitmap);
    log_destroy(logger);
}

void mutexDestroy(void* ptr){
    pthread_mutex_t* mutex = (pthread_mutex_t*) ptr;
    pthread_mutex_destroy(mutex);
    free(mutex);
}

void retardo_operacion(){
    usleep(configuracion->retardoOperacion * 1000);
}

void retardo_bloque(){
    usleep(configuracion->retardoAccesoBloque * 1000);
}
