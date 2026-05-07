#include <utils/cliente.h>

int iniciarCliente(char *ip, char *puerto) {
    struct addrinfo hints;
    struct addrinfo *server_info;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int rv = getaddrinfo(ip, puerto, &hints, &server_info);
    if (rv != 0) {
        return -1;
    }

    int socket_cliente = socket(server_info->ai_family,
                                server_info->ai_socktype,
                                server_info->ai_protocol);
    if (socket_cliente == -1) {
        freeaddrinfo(server_info);
        return -1;
    }

    if (connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen) == -1) {
        close(socket_cliente);
        freeaddrinfo(server_info);
        return -1;
    }

    freeaddrinfo(server_info);
    return socket_cliente;
}

int32_t handshakeCliente(int conexion){
	int32_t handshake = 1;
	int32_t result;
	send(conexion, &handshake, sizeof(int32_t), 0);
	recv(conexion, &result, sizeof(int32_t), MSG_WAITALL);
	return result;
}