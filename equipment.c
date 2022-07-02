#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUFSZ 500

struct equipamento {
    int socketEquip;
};

void interpretar_req_rem(char *idEquipamento) {

	printf("Equipment %s removed\n", idEquipamento);
}

void interpretar_req_add(char *novoId) {

    printf("Equipment %s added\n", novoId);
}

void interpretar_res_list(char *payload){

	printf("%s\n", payload);
}

void interpretar_erro(int socketEquipamento, char *idDestino, char *payload) {
   
   switch(atoi(payload)) {
	case 1:
		printf("Equipment not found\n");
		break;
	case 4:
		printf("Equipment limit exceeded\n");
		break;
	default:
		break;
    }
	close(socketEquipamento);
	exit(EXIT_SUCCESS);
}

void interpretar_res_add(char *payload){
   
	int idEquipamento = atoi(payload);
	if(idEquipamento < 10)
        printf("New ID: 0%d\n", idEquipamento);
    else
        printf("New ID: %d\n", idEquipamento);

}

void interpretar_ok(int socketEquipamento, char *payload) {
	
	if(atoi(payload) == 1) {
		printf("Successful removal\n");
		close(socketEquipamento);
		exit(EXIT_SUCCESS);
	}
}

void interpretar_resposta(char buffer[BUFSZ], int socketEquipamento){
	
	char *idMsg = strtok(buffer, ",");
	char *idOrigem = strtok(NULL, ",");
	char *idDestino = strtok(NULL, ",");
	char *payload = strtok(NULL, "");

	switch (atoi(idMsg)) {	
	case 1:
		interpretar_req_add(idOrigem);
		break;
	case 2:
		interpretar_req_rem(idOrigem);
		break;
	case 3:
		interpretar_res_add(payload);
		break;
	case 4:
		interpretar_res_list(payload);
		break;
	case 7:
		interpretar_erro(socketEquipamento, idDestino, payload);
		break;
	case 8:
		interpretar_ok(socketEquipamento, payload);
		break;
	default:
		break;
	}
}

void exibir_instrucoes_de_uso(int argc, char **argv) {
    printf("Uso: %s <IP do servidor> <Porta do servidor>\n", argv[0]);
    printf("Exemplo: %s 127.0.0.1 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

void * client_thread(void *data) {
    struct equipamento *equipData = (struct equipamento *)data;

	while(1){
		char buffer[BUFSZ];
		memset(buffer, 0, BUFSZ);
		recv(equipData->socketEquip, buffer, BUFSZ + 1, 0);
		if(strlen(buffer)!= 0){
			interpretar_resposta(buffer,equipData->socketEquip);
		}
	}
	exit(EXIT_SUCCESS);
}


int main(int argc, char **argv) {

	if (argc < 3) {
		exibir_instrucoes_de_uso(argc, argv);
	}

	struct sockaddr_storage storage;
	if (analisar_endereco(argv[1], argv[2], &storage) != 0) {
		exibir_instrucoes_de_uso(argc, argv);
	}

	int _socket;
	_socket = socket(storage.ss_family, SOCK_STREAM, 0);
	if (_socket == -1) {
		exibir_log_saida("socket");
	}
	struct sockaddr *endereco = (struct sockaddr *)(&storage);
	if (connect(_socket, endereco, sizeof(storage)) != 0) {
		exibir_log_saida("connect");
	}

	char enderecoStr[BUFSZ];
	converter_endereco_em_string(endereco, enderecoStr, BUFSZ);
	printf("connected to %s\n", enderecoStr);

	char buffer[BUFSZ];
	memset(buffer, 0, BUFSZ);
	recv(_socket, buffer, BUFSZ, 0);
	interpretar_resposta(buffer, _socket);

	while(1) {
		memset(buffer, 0, BUFSZ);
		struct equipamento *equipData = malloc(sizeof(*equipData));
		if (!equipData) {
			exibir_log_saida("malloc");
		}
		equipData->socketEquip = _socket;

		pthread_t tid;
		pthread_create(&tid, NULL, client_thread, equipData);

		memset(buffer, 0, BUFSZ);
		fgets(buffer, BUFSZ-1, stdin);
		size_t numBytes = send(_socket, buffer, strlen(buffer)+1, 0);
		if (numBytes != strlen(buffer)+1) {
			exibir_log_saida("send");
		}
	}

	exit(EXIT_SUCCESS);
}
