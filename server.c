#include "common.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>

#define BUFSZ 500
#define MAX_EQUIPAMENTOS 15

bool equipamentos[MAX_EQUIPAMENTOS] = { false };
int numEquipamentos = 0;

struct equipment_data {
	int equipmentSocket;
	struct sockaddr_storage storage;
};

void print_equipamento_adicionado(char* idEquipamento) {
    if(atoi(idEquipamento) < 10)
        printf("Equipment 0%s added\n", idEquipamento);
    else
        printf("Equipment %s added\n", idEquipamento);
}

void adicionar_equipamento(int socketEquipamento) {
    char idEquipamento[4];
    char msg[500];
    memset(msg, 0, 500);
    if(numEquipamentos == MAX_EQUIPAMENTOS)
        strcat(msg, "07,04, , ");
    else{
        for(int posicao = 8; posicao < MAX_EQUIPAMENTOS; posicao++){
            if(!equipamentos[posicao]){
                equipamentos[posicao] = true;
                sprintf(idEquipamento, "%d", posicao + 1);
		printf("OLHA idEquipment: %s\n", idEquipamento);
                strcat(msg, "03,");
		printf("OLHA msg: %s\n", msg);
                strcat(msg, idEquipamento);
		printf("OLHA idEquipment: %s\n", idEquipamento);
		printf("OLHA msg: %s\n", msg);
		strcat(msg, ", , ");
		printf("OLHA msg: %s\n", msg);
                printf("OLHA idEquipment: %s\n", idEquipamento);
		print_equipamento_adicionado(idEquipamento);
                numEquipamentos++;
                break;
            }
        }
    }
    
    int numBytes = send(socketEquipamento, msg, strlen(msg), 0); //Envia a mensagem
    if(numBytes != strlen(msg))
        exibirLogSaida("send");

}

void *client_thread(void *data) {

	struct equipment_data *cdata = (struct equipment_data *) data;
	struct sockaddr *caddr = (struct sockaddr *) (&cdata->storage);

	char caddrstr[BUFSZ];
	converterEnderecoEmString(caddr, caddrstr, BUFSZ);
	printf("[log] connection from %s\n", caddrstr);

	char buff[BUFSZ];
        adicionar_equipamento(cdata->equipmentSocket);
	int auxRetorno = 0;
	unsigned totalBytes;
	size_t numBytes;
	char *linhas;
	while(1){
	    memset(buff, 0, BUFSZ);
	    totalBytes = 0;
	    while(buff[strlen(buff)-1] != '\n') {
	        numBytes = recv(cdata->equipmentSocket, buff + totalBytes, BUFSZ - totalBytes, 0);
	        if(numBytes == 0){
	            auxRetorno = -1;
	            break;
	        }
	        totalBytes += numBytes;
	    }
	    if(auxRetorno == -1)
	        break;
	    printf("%s", buff);
	    linhas = strtok(buff, "\n");
	    if(strcmp(buff, "kill") == 0){
	        close(cdata->equipmentSocket);
	        exit(EXIT_SUCCESS);
	        return 0;
	    }

	    while(linhas != NULL){
		//auxRetorno = avaliarComando(buff, cdata->equipmentSocket);
		if(auxRetorno == -1){
	            close(cdata->equipmentSocket);
	            exit(EXIT_SUCCESS);
	            return 0;
	        }
	        linhas = strtok(NULL, "\n");
	    }
	    if(auxRetorno == -1)
	        break;
	}
	close(cdata->equipmentSocket);

	pthread_exit(EXIT_SUCCESS);
}

void exibirInstrucoesDeUso(int argc, char **argv) {
    printf("Uso: %s <Porta do servidor>\n", argv[0]);
    printf("Exemplo: %s 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    srand((unsigned) time(NULL));
    if (argc < 2) {
        exibirInstrucoesDeUso(argc, argv);
    }

    struct sockaddr_storage storage;
    if (0 != inicializarSockAddrServer(argv[1], &storage)) {
        exibirInstrucoesDeUso(argc, argv);
    }

    int socket_;
    socket_ = socket(storage.ss_family, SOCK_STREAM, 0);
    if (socket_ == -1) {
        exibirLogSaida("socket");
    }

    int enable = 1;
    if (0 != setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))) {
        exibirLogSaida("setsockopt");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (0 != bind(socket_, addr, sizeof(storage))) {
        exibirLogSaida("bind");
    }

    if (0 != listen(socket_, 10)) {
        exibirLogSaida("listen");
    }

    char addrstr[BUFSZ];
    converterEnderecoEmString(addr, addrstr, BUFSZ);
    printf("bound to %s, waiting connections\n", addrstr);
    //memset(equipamentos, 0, sizeof(equipamentos)); //Resetando o array de equipamentos
    
    while (1) {
        struct sockaddr_storage cstorage;
        struct sockaddr *caddr = (struct sockaddr *)(&cstorage);
        socklen_t caddrlen = sizeof(cstorage);

	int socketCliente = accept(socket_, caddr, &caddrlen);
	if (socketCliente == -1) {
	    exibirLogSaida("accept");
	}
	struct equipment_data *cdata = malloc(sizeof(*cdata));
	if(!cdata)
	    exibirLogSaida("malloc");
	cdata->equipmentSocket = socketCliente;
	memcpy(&(cdata->storage), &cstorage, sizeof(cstorage));

	pthread_t tid;
	pthread_create(&tid, NULL, client_thread, cdata);
    }

    exit(EXIT_SUCCESS);
}


