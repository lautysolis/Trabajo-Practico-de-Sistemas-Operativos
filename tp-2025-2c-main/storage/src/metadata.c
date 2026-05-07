#include "metadata.h"

void crear_meta(char *nombreFile, char *tag, int32_t tamanio_inicial){

    char *metadataConfig = path_tag_metadata(nombreFile, tag);
    FILE *file_meta = fopen(metadataConfig, "wb");
    if (!file_meta)
    {
        log_error(logger, "error al crear metadata.config para %s:%s", nombreFile, tag);
        free(metadataConfig);
        return;
    }
    fclose(file_meta);

    t_config *config_meta = iniciar_config(metadataConfig);

    char *tamanio_str = string_from_format("%d", tamanio_inicial);
    config_set_value(config_meta, "TAMAÑO", tamanio_str);
    config_set_value(config_meta, "BLOCKS", "[]");
    config_set_value(config_meta, "ESTADO", "WORK_IN_PROGRESS");

    config_save(config_meta);
    config_destroy(config_meta);

    free(tamanio_str);
    free(metadataConfig);
}

int cantidad_bloques_meta(t_instruccion *instruccion){

    char *metadataConfig = path_tag_metadata(instruccion->nombreFile, instruccion->tag);

    t_config *config_meta = iniciar_config(metadataConfig);

    char *bloques_str = config_get_string_value(config_meta, "BLOCKS");
    t_list *listaBloques = parsear_lista_enteros(bloques_str);

    int cantidad = list_size(listaBloques);

    free(metadataConfig);
    config_destroy(config_meta);
    list_destroy_and_destroy_elements(listaBloques, free);

    return cantidad;
}


t_list *obtener_bloques_meta(t_instruccion *instruccion){

    char *metadataConfig = path_tag_metadata(instruccion->nombreFile, instruccion->tag);

    t_config *config_meta = iniciar_config(metadataConfig);

    char *bloques_str = config_get_string_value(config_meta, "BLOCKS");
    t_list *listaBloques = parsear_lista_enteros(bloques_str);

    free(metadataConfig);
    config_destroy(config_meta);

    return listaBloques;
}

void modificar_bloques_meta(char* nombreFile, char* tag, t_list* bloques){

    char *metadataConfig = path_tag_metadata(nombreFile, tag);

    t_config *config_meta = iniciar_config(metadataConfig);

    char *nuevaLista = lista_enteros_a_string(bloques);

    config_set_value(config_meta,"BLOCKS", nuevaLista);
    config_save(config_meta);
    config_destroy(config_meta);

    free(metadataConfig);
    free(nuevaLista);
}


void cambiar_tamanio_meta(char *nombreFile, char *tag, int tamanio){
    char *metadataConfig = path_tag_metadata(nombreFile, tag);

    t_config *config_meta = iniciar_config(metadataConfig);

    char *tamanio_str = string_from_format("%d", tamanio);

    config_set_value(config_meta, "TAMAÑO", tamanio_str);
    config_save(config_meta);
    config_destroy(config_meta);

    free(metadataConfig);
    free(tamanio_str);
}

int pasar_estado_commited_meta(char* nombreFile, char* tag){

    char *metadataConfig = path_tag_metadata(nombreFile, tag);

    int commiteado = 0;

    t_config *config_meta = iniciar_config(metadataConfig);

    char* estado = config_get_string_value(config_meta,"ESTADO");

    if(strcmp(estado,"WORK_IN_PROGRESS") == 0){    
        config_set_value(config_meta, "ESTADO", "COMMITED");
        config_save(config_meta);
        commiteado = 1;
    }

    config_destroy(config_meta);

    free(metadataConfig);

    return commiteado;
}

int verificarEstadoCommited(char* nombreFile, char* tag){

    char *metadataConfig = path_tag_metadata(nombreFile, tag);

    t_config *config_meta = iniciar_config(metadataConfig);

    char* estado = config_get_string_value(config_meta,"ESTADO");

    int comitteado = strcmp(estado,"COMMITED") == 0;;

    config_destroy(config_meta);
    free(metadataConfig);
    
    return comitteado;
}
