#include <utils/utils.h>

t_config* iniciar_config(char* archivo_config)
{
	t_config* nuevo_config = config_create(archivo_config);

	if (nuevo_config == NULL) {
        log_error(logger,"ERROR: FIN PROGRAMA ERROR AL CREAR EL CONFIG");
        abort();
	}
	return nuevo_config;
}

int validarEntero(char* texto,char* error){
	char* caracterFin;
    int numero = strtol(texto, &caracterFin, 10);

    if (*caracterFin != '\0') {
        printf("%s",error);
    }
	return numero;
}


bool stringToBool(char* str){
	if (str == NULL){
	return false;	
	} 

    if (strcmp(str, "TRUE") == 0){ 
    return true;
	} 
        
    if (strcmp(str, "FALSE") == 0){
	return false;
	}
        
return false;
}


int* intdup(int bloqueFisico){
	int *bloque = malloc(sizeof(int));
	*bloque = bloqueFisico;
	
	return bloque;
}

t_list* parsear_lista_enteros(char* str) {

    char* copia = string_duplicate(str);

    copia++; // saca [
    copia[string_length(copia)-1] = '\0'; // saca el ']'

    char** tokens = string_split(copia, ",");

    t_list* lista = list_create();

    for (int i = 0; tokens[i] != NULL; i++) {
        int* valor = intdup(atoi(tokens[i]));
        list_add(lista, valor);
    }
    
    string_array_destroy(tokens);
    copia--;
    free(copia);

    return lista;
}

char* lista_enteros_a_string(t_list* lista) {
    char* resultado = string_new();
    string_append(&resultado, "[");

    for(int i = 0; i < list_size(lista); i++){
        int* p_valor = list_get(lista, i);
        int valor = *p_valor;
        if(i > 0)
            string_append(&resultado, ",");
        char* str_valor = string_itoa(valor);  
        string_append(&resultado, str_valor);
        free(str_valor);
    }

    string_append(&resultado, "]");
    return resultado;
}

