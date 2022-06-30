#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

void exibirInstrucoesDeUso(int argc, char **argv) {
    printf("Uso: %s <IP do servidor> <Porta do servidor>\n", argv[0]);
    printf("Exemplo: %s 127.0.0.1 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

#define BUFFERSIZE 500

void interpretar_res_add(char *payload) {

    if(atoi(payload) < 10)
        printf("New ID: 0%s\n", payload);
    else
        printf("New ID: %s\n", payload);
}

void interpretar_erro(int socketEquipamento, char* idDestino, char* payload) {

    switch(atoi(payload)) {
        case 4:
            printf("Equipment limit exceeded\n");
            close(socketEquipamento);
            break;
        default:
            break;

    }
}

void interpretar_resposta(char buff[BUFFERSIZE], int socketEquipamento) {
    
    char *idMsg = strtok(buff, ",");
    char *payload = strtok(NULL, ",");
    char *idOrigem = strtok(NULL, ",");
    char *idDestino = strtok(NULL, ",");

    switch(atoi(idMsg)) {
        case 3:
            interpretar_res_add(payload);
            break;
        case 7:
            interpretar_erro(socketEquipamento, idDestino, payload);
            break;
        default:
            break;
    }

}


int main(int argc, char **argv) {

    if(argc < 3) 
        exibirInstrucoesDeUso(argc, argv);

    struct sockaddr_storage storage;
    if(parsearEndereco(argv[1], argv[2], &storage) != 0)
        exibirInstrucoesDeUso(argc, argv);
    
    int socket_;
    socket_ = socket(storage.ss_family, SOCK_STREAM, 0);
    if(socket_ == -1)
        exibirLogSaida("socket");
    
    struct sockaddr *endereco = (struct sockaddr *) (&storage);
    if(connect(socket_, endereco, sizeof(storage)) != 0)
        exibirLogSaida("connect");

    char addrstr[BUFFERSIZE];
    converterEnderecoEmString(endereco, addrstr, BUFFERSIZE);
    printf("connected to %s\n", addrstr);
    char buff[BUFFERSIZE];
    memset(buff, 0, BUFFERSIZE);
    recv(socket_, buff, BUFFERSIZE, 0);
    interpretar_resposta(buff, socket_);

    while(1){
	printf("> ");
	memset(buff, 0, BUFFERSIZE);
	size_t numBytes;
	while(buff[strlen(buff)-1] != '\n'){
	    memset(buff, 0, BUFFERSIZE);
	    fgets(buff, BUFFERSIZE-1, stdin);
	    numBytes = send(socket_, buff, strlen(buff), 0);
	    if (numBytes != strlen(buff)) {
	        exibirLogSaida("send");
   	    }
	}

	if(strcmp(buff, "kill\n") == 0){
	    close(socket_);
	    exit(EXIT_SUCCESS);
	}
	memset(buff, 0, BUFFERSIZE);
	unsigned totalBytes = 0;
	while(buff[strlen(buff)-1] != '\n') {
	    numBytes = recv(socket_, buff + totalBytes, BUFFERSIZE - totalBytes, 0);
            //interpretar_resposta(buff, socket_);
	    if(numBytes == 0) {
	        break; // Fecha a conexão
	    }
	    totalBytes += numBytes;
	}
	printf("< %s", buff);
    }
    close(socket_);

    exit(EXIT_SUCCESS);
}

