
#include <utils/servidor.h>


int iniciarServidor(char* puerto)
{
	int socket_servidor;

	struct addrinfo hints, *servinfo; 

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(NULL, puerto, &hints, &servinfo);

	socket_servidor = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
	
	setsockopt(socket_servidor, SOL_SOCKET, SO_REUSEPORT, &(int){1}, sizeof(int));
	
	bind(socket_servidor, servinfo->ai_addr, servinfo->ai_addrlen);

	listen(socket_servidor, SOMAXCONN);

	freeaddrinfo(servinfo);

	return socket_servidor;
}

void esperarCliente(int socketServer, void* funcion)
{
	while (true) {
        pthread_t thread;
        int *socketCliente = malloc(sizeof(int));
        *socketCliente = accept(socketServer, NULL, NULL);
        pthread_create(&thread,NULL,funcion,socketCliente);
        pthread_detach(thread);
    }
}

void handshakeServidor(int socketCliente){
    int32_t handshake;
    int32_t resultOk = 0;
    int32_t resultError = -1;

    recv(socketCliente, &handshake, sizeof(int32_t), MSG_WAITALL);

    if (handshake == 1) {
        send(socketCliente, &resultOk, sizeof(int32_t), 0);
    } else {
        send(socketCliente, &resultError, sizeof(int32_t), 0);
    }
}
