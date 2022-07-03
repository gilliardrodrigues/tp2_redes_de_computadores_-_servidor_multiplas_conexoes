#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#define TAMANHO_BUFFER 500
#define REMOVIDO_COM_SUCESSO 1

struct equipamento {
    int socketEquip;
};

enum tipos_de_erro {
	EQUIPAMENTO_NAO_ENCONTRADO = 1, 
	EQUIPAMENTO_SOLICITANTE_NAO_ENCONTRADO, 
	EQUIPAMENTO_REQUISITADO_NAO_ENCONTRADO, 
	NUM_MAXIMO_DE_EQUIPAMENTOS_ALCANCADO
} erros;

enum tipos_de_resposta {
	REQ_ADD = 1,
	REQ_REM,
	RES_ADD,
	RES_LIST,
	REQ_INF,
	RES_INF,
	ERROR,
	OK
} respostas;

void interpretar_res_inf(char *idEquipamento, char *informacao) {

	printf("Value from %s: %s\n", idEquipamento, informacao);
}

void interpretar_req_inf() {

	printf("requested information\n");
}

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
   
	erros = atoi(payload);

   switch(erros) {
	case EQUIPAMENTO_NAO_ENCONTRADO:
		printf("Equipment not found\n");
		break;
	case EQUIPAMENTO_SOLICITANTE_NAO_ENCONTRADO:
		printf("Source equipment not found\n");
		break;
	case EQUIPAMENTO_REQUISITADO_NAO_ENCONTRADO:
		printf("Target equipment not found\n");
		break;
	case NUM_MAXIMO_DE_EQUIPAMENTOS_ALCANCADO:
		printf("Equipment limit exceeded\n");
		break;
	default:
		break;
    }
}

void interpretar_res_add(char *payload){
   
	int idEquipamento = atoi(payload);

	if(idEquipamento < 10)
        printf("New ID: 0%d\n", idEquipamento);
    else
        printf("New ID: %d\n", idEquipamento);

}

void interpretar_ok(int socketEquipamento, char *payload) {
	
	if(atoi(payload) == REMOVIDO_COM_SUCESSO) {
		printf("Successful removal\n");
		close(socketEquipamento);
		exit(EXIT_SUCCESS);
	}
}

void interpretar_resposta(char buffer[TAMANHO_BUFFER], int socketEquipamento){
	
	char *idMsg = strtok(buffer, ",");
	char *idOrigem = strtok(NULL, ",");
	char *idDestino = strtok(NULL, ",");
	char *payload = strtok(NULL, "\n");

	respostas = atoi(idMsg);

	switch (respostas) {	
	case REQ_ADD:
		interpretar_req_add(idOrigem);
		break;
	case REQ_REM:
		interpretar_req_rem(idOrigem);
		break;
	case RES_ADD:
		interpretar_res_add(payload);
		break;
	case RES_LIST:
		interpretar_res_list(payload);
		break;
	case REQ_INF:
		interpretar_req_inf();
		break;
	case RES_INF:
		interpretar_res_inf(idOrigem, payload);
		break;
	case ERROR:
		interpretar_erro(socketEquipamento, idDestino, payload);
		break;
	case OK:
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
		char buffer[TAMANHO_BUFFER];
		memset(buffer, 0, TAMANHO_BUFFER);
		recv(equipData->socketEquip, buffer, TAMANHO_BUFFER + 1, 0);
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

	char enderecoStr[TAMANHO_BUFFER];
	converter_endereco_em_string(endereco, enderecoStr, TAMANHO_BUFFER);
	//printf("connected to %s\n", enderecoStr);

	char buffer[TAMANHO_BUFFER];
	memset(buffer, 0, TAMANHO_BUFFER);
	recv(_socket, buffer, TAMANHO_BUFFER, 0);
	interpretar_resposta(buffer, _socket);

	while(1) {
		memset(buffer, 0, TAMANHO_BUFFER);
		struct equipamento *equipData = malloc(sizeof(*equipData));
		if (!equipData) {
			exibir_log_saida("malloc");
		}
		equipData->socketEquip = _socket;

		pthread_t tid;
		pthread_create(&tid, NULL, client_thread, equipData);

		memset(buffer, 0, TAMANHO_BUFFER);
		fgets(buffer, TAMANHO_BUFFER-1, stdin);
		size_t numBytes = send(_socket, buffer, strlen(buffer)+1, 0);
		if (numBytes != strlen(buffer)+1) {
			exibir_log_saida("send");
		}
	}

	exit(EXIT_SUCCESS);
}
