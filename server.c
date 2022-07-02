#include "common.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>

#define BUFSZ 500
#define MAX_EQUIPAMENTOS 15  

int sockets[MAX_EQUIPAMENTOS] = { 0 };
bool equipamentos[MAX_EQUIPAMENTOS] = { false };
int numEquipamentos = 0;

struct equipamento {
        int equipSocket;
        struct sockaddr_storage storage;
};

void listar_equipamentos(char buffer[BUFSZ], int idSolicitante){

    memset(buffer, 0, BUFSZ);

    char *lista = malloc(sizeof(char)*BUFSZ);
    char idEquipamento[4];
    bool temEquipamento = false;

    for(int posicao = 0; posicao < MAX_EQUIPAMENTOS; posicao++) {
        if(equipamentos[posicao] && (posicao + 1) != idSolicitante) {
            temEquipamento = true;
            if(posicao < 10) {
                sprintf(idEquipamento, "0%d ", posicao + 1);
                strcat(lista, idEquipamento);
            }
            else {
                sprintf(idEquipamento, "%d ", posicao + 1);
                strcat(lista, idEquipamento);
            }
        }
    }

    if (!temEquipamento)
        sprintf(buffer, "7,-,-,1");
    else
        sprintf(buffer, "4,-,-,%s", lista);
}

bool pode_enviar_broadcast(int indice, int idEquipamento) {

    return (sockets[indice] != 0 && (indice + 1) != idEquipamento);
}

void enviar_broadcast(char buffer[BUFSZ], char* idEquipamento) {

    int id = atoi(idEquipamento);
    for(int posicao = 0; posicao < MAX_EQUIPAMENTOS; posicao++)
        if(pode_enviar_broadcast(posicao, id))
            send(sockets[posicao], buffer, strlen(buffer) + 1, 0);
}

char* formatar_id_equipamento(char* idEquipamento) {
    if(atoi(idEquipamento) < 10){
        char *idModificado = malloc(sizeof(char)*4);
        strcpy(idModificado, "0");
        strcat(idModificado, idEquipamento);
        return idModificado;
    }
    return idEquipamento;
}

void remover_equipamento(int idSolicitante) {

    int numBytes;
    char msg[BUFSZ];
    memset(msg, 0, BUFSZ);
    char *idEquipamento = malloc(sizeof(char)*4);
    sprintf(idEquipamento, "%d", idSolicitante);
    idEquipamento = formatar_id_equipamento(idEquipamento);
    int socketEquipamento = sockets[idSolicitante-1];

    if(!equipamentos[idSolicitante-1]) {
        strcat(msg, "07,-,");
        strcat(msg, idEquipamento);
        strcat(msg, ",01");
        numBytes = send(socketEquipamento, msg, strlen(msg)+1, 0);
    }
    else {
        strcat(msg, "08,-,");
        strcat(msg, idEquipamento);
        strcat(msg, ",01");
        printf("Equipment %s removed\n", idEquipamento);
        numEquipamentos--;
        numBytes = send(socketEquipamento, msg, strlen(msg)+1, 0);
        memset(msg, 0, BUFSZ);
        sprintf(msg, "02,%s,-,-", idEquipamento);
        enviar_broadcast(msg, idEquipamento);
        close(sockets[idSolicitante-1]);
        sockets[idSolicitante-1] = 0;
        equipamentos[idSolicitante-1] = false;
        pthread_exit(EXIT_SUCCESS);
    }
    if(numBytes != strlen(msg)+1) {
        exibir_log_saida("send");
    }
}

int adicionar_equipamento(char buffer[BUFSZ], int socketEquipamento) {

    char *idEquipamento = malloc(sizeof(char)*4);
    strcpy(idEquipamento, "ne"); // not exist

    char msg[BUFSZ];
    memset(msg, 0, BUFSZ);
    if(numEquipamentos == MAX_EQUIPAMENTOS)
        strcat(msg, "07,-,-,04");
    else{
        for(int posicao = 0; posicao < MAX_EQUIPAMENTOS; posicao++){
            if(!equipamentos[posicao]) {
                equipamentos[posicao] = true;
                sockets[posicao] = socketEquipamento;
                sprintf(idEquipamento, "%d", posicao + 1);
                strcat(msg, "03,-,-,");
                strcat(msg, idEquipamento);
                idEquipamento = formatar_id_equipamento(idEquipamento);
                sprintf(buffer, "Equipment %s added\n", idEquipamento);
                numEquipamentos++;
                break;
            }
        }
    }

    int numBytes = send(socketEquipamento, msg, strlen(msg)+1, 0);

    if(numBytes != strlen(msg)+1)
        exibir_log_saida("send");
    if(strcmp(idEquipamento, "ne") != 0 && numEquipamentos > 1) {
        memset(msg, 0, BUFSZ);
        sprintf(msg, "01,%s,-,-", idEquipamento);
        enviar_broadcast(msg, idEquipamento);
    }
    return atoi(idEquipamento);
}

void interpretar_entrada(char buffer[BUFSZ], int idEquipamento){
    
    char aux[BUFSZ];
    memset(aux, 0, BUFSZ);
    strcpy(aux, buffer);
    strtok(aux, " ");
    if(strcmp(buffer, "list equipments\n") == 0)
        listar_equipamentos(buffer, idEquipamento);
    else if(strcmp(buffer, "close connection\n") == 0)
        remover_equipamento(idEquipamento);
}

void exibir_instrucoes_de_uso(int argc, char **argv) {
    printf("Uso: %s <Porta do servidor>\n", argv[0]);
    printf("Exemplo: %s 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

void * client_thread(void *data) {
    struct equipamento *equipData = (struct equipamento *) data;
    struct sockaddr *enderecoEquip = (struct sockaddr *) (&equipData->storage);

    int idEquipamento = 0;
    char enderecoEquipStr[BUFSZ];
    converter_endereco_em_string(enderecoEquip, enderecoEquipStr, BUFSZ);
    //printf("[log] connection from %s\n", enderecoEquipStr);

    char buffer[BUFSZ];
    memset(buffer, 0, BUFSZ);
    
    char resposta[BUFSZ];
    memset(resposta, 0, BUFSZ);
    
    idEquipamento = adicionar_equipamento(buffer, equipData->equipSocket);
    if(!idEquipamento){
       close(sockets[idEquipamento-1]);
       pthread_exit(EXIT_SUCCESS);
    }
    printf("%s\n", buffer);
    
    while(1){
        memset(buffer, 0, BUFSZ);
        recv(sockets[idEquipamento-1], buffer, BUFSZ - 1, 0);
        memset(resposta, 0, BUFSZ);
        interpretar_entrada(buffer, idEquipamento);
        
        int numBytes = send(sockets[idEquipamento-1], buffer, strlen(buffer) + 1, 0);
        if (numBytes != strlen(buffer) + 1) {
            exibir_log_saida("send");
        }
    }
    close(equipData->equipSocket);

    pthread_exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        exibir_instrucoes_de_uso(argc, argv);
    }
    struct sockaddr_storage storage;
    if (0 != inicializar_sock_addr_server(argv[1], &storage)) {
        exibir_instrucoes_de_uso(argc, argv);
    }

    int _socket;
    _socket = socket(storage.ss_family, SOCK_STREAM, 0);
    if (_socket == -1) {
        exibir_log_saida("socket");
    }

    int enable = 1;
    if (setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) != 0) {
        exibir_log_saida("setsockopt");
    }

    struct sockaddr *endereco = (struct sockaddr *)(&storage);
    if (bind(_socket, endereco, sizeof(storage)) != 0) {
        exibir_log_saida("bind");
    }

    if (listen(_socket, 10) != 0) {
        exibir_log_saida("listen");
    }

    char enderecoStr[BUFSZ];
    converter_endereco_em_string(endereco, enderecoStr, BUFSZ);

    while (1) {
        struct sockaddr_storage cstorage;
        struct sockaddr *enderecoEquip = (struct sockaddr *)(&cstorage);
        socklen_t tamanhoEnderecoEquip = sizeof(cstorage);

        int equipSocket = accept(_socket, enderecoEquip, &tamanhoEnderecoEquip);
        if (equipSocket == -1) {
            exibir_log_saida("accept");
        }

	struct equipamento *equipData = malloc(sizeof(*equipData));
	if (!equipData) {
		exibir_log_saida("malloc");
	}
	equipData->equipSocket = equipSocket;
	memcpy(&(equipData->storage), &cstorage, sizeof(cstorage));

        pthread_t tid;
        pthread_create(&tid, NULL, client_thread, equipData);
    }

    exit(EXIT_SUCCESS);
}

