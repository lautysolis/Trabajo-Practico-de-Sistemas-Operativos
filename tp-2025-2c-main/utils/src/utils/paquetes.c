#include <utils/paquetes.h>
#include <utils/estructuras.h>

t_buffer* crearBuffer(int32_t size){
	t_buffer* buffer = malloc(sizeof(t_buffer));
	buffer->size = size;
	buffer->offset = 0;
	buffer->stream = malloc(size);
	return buffer;
}

void agregarBuffer(t_buffer *buffer, void* data, int32_t size){
    memcpy(buffer->stream + buffer->offset, data, size);
    buffer->offset += size;
};

void leerBuffer(t_buffer *buffer, void *data, int32_t size){
	memcpy(data,buffer->stream + buffer->offset, size);
    buffer->offset += size;
};

void enviarPaquete(int socket,codigo_operacion codigoOperacion, t_buffer* buffer){
	t_paquete* paquete = crearPaquete(codigoOperacion,buffer);

	void* a_enviar = malloc(paquete->buffer->size + sizeof(codigo_operacion) + sizeof(int32_t));
	int offset = 0;

	memcpy(a_enviar + offset, &(paquete->codigoOperacion), sizeof(codigo_operacion));
	offset += sizeof(codigo_operacion);
	
	memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(int32_t));
	offset += sizeof(int32_t);
	
	memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);

	send(socket, a_enviar, buffer->size + sizeof(codigo_operacion) + sizeof(int32_t), 0);

	free(a_enviar);
	eliminarPaquete(paquete);
}

t_paquete* crearPaquete(codigo_operacion codigoOperacion,t_buffer* buffer){
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigoOperacion = codigoOperacion; 
	paquete->buffer = buffer;
	return paquete;
}

void eliminarPaquete(t_paquete* paquete){
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}

t_paquete* recibirPaquete(int socket){
    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->buffer = malloc(sizeof(t_buffer));
	if (recv(socket, &(paquete->codigoOperacion), sizeof(codigo_operacion), MSG_WAITALL) <= 0){
		free(paquete->buffer);
		free(paquete);
		return NULL;
	}
	if (recv(socket, &(paquete->buffer->size), sizeof(int32_t), MSG_WAITALL) <= 0){
		free(paquete->buffer);
		free(paquete);
    	return NULL;
	}
	paquete->buffer->stream = malloc(paquete->buffer->size);
	if (recv(socket, paquete->buffer->stream, paquete->buffer->size, MSG_WAITALL) <= 0){
		free(paquete->buffer->stream);
		free(paquete->buffer);
		free(paquete);
    	return NULL;
	}
    return paquete;
}
