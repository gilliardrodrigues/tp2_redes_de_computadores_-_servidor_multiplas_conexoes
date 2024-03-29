#include "common.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>

#define TAMANHO_BUFFER 500
#define MAX_EQUIPAMENTOS 15
#define TAMANHO_ID 4 // Considerando espaços
#define EQUIPAMENTO_NAO_EXISTE "ne"

int sockets[MAX_EQUIPAMENTOS] = { 0 };
bool equipamentos[MAX_EQUIPAMENTOS] = { false };
int numEquipamentos = 0;

struct equipamento {
        int equipSocket;
        struct sockaddr_storage storage;
};

bool equipamento_existe_e_nao_eh_o_solicitante(bool equipamentos[MAX_EQUIPAMENTOS], int posicao, int idSolicitante) {

    return (equipamentos[posicao] && (posicao + 1) != idSolicitante);
}

void listar_equipamentos(char buffer[TAMANHO_BUFFER], int idSolicitante){

    memset(buffer, 0, TAMANHO_BUFFER);

    char *lista = malloc(sizeof(char)*TAMANHO_BUFFER);
    char idEquipamento[TAMANHO_ID];
    bool temEquipamento = false;

    for(int posicao = 0; posicao < MAX_EQUIPAMENTOS; posicao++) {
        if(equipamento_existe_e_nao_eh_o_solicitante(equipamentos, posicao, idSolicitante)) {
            temEquipamento = true;
            if(posicao + 1 < 10) {
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

void enviar_broadcast(char buffer[TAMANHO_BUFFER], char* idEquipamento) {

    int id = atoi(idEquipamento);
    for(int posicao = 0; posicao < MAX_EQUIPAMENTOS; posicao++)
        if(pode_enviar_broadcast(posicao, id))
            send(sockets[posicao], buffer, strlen(buffer) + 1, 0);
}

char* formatar_id_equipamento(char* idEquipamento) {
    if(atoi(idEquipamento) < 10){
        char *idModificado = malloc(sizeof(char)*TAMANHO_ID);
        strcpy(idModificado, "0");
        strcat(idModificado, idEquipamento);
        return idModificado;
    }
    return idEquipamento;
}

double gerar_leitura_equipamento() {
    // Gera um valor aleatório entre 0.00 e 10.00:
    return (double)(rand() % 1001)/100;
}

void enviar_informacao_equipamento(char buffer[TAMANHO_BUFFER], char *idSolicitante, char *idRequisitado) {
    double informacao = gerar_leitura_equipamento();
    sprintf(buffer, "6,%s,%s,%.2f", idRequisitado, idSolicitante, informacao);
}

bool equipamento_existe(int idEquipamento) {

    return equipamentos[idEquipamento-1];
}

void requisitar_informacao_equipamento(char buffer[TAMANHO_BUFFER], int idEquipamento) {
    
    // String que guardará a mensagem:
    char msg[TAMANHO_BUFFER];
    memset(msg, 0, TAMANHO_BUFFER);

    // Pegando o id do solicitante formatado:
    char *idSolicitante = malloc(sizeof(char)*TAMANHO_ID);
    sprintf(idSolicitante, "%d", idEquipamento);
    idSolicitante = formatar_id_equipamento(idSolicitante);

    // Pegando o id do equipamento requistado:
    char *idRequisitado = malloc(sizeof(char)*TAMANHO_BUFFER);
    memset(idRequisitado, 0, TAMANHO_BUFFER);
    idRequisitado = strtok(buffer, " ");
    idRequisitado = strtok(NULL, " ");
    idRequisitado = strtok(NULL, " ");
    idRequisitado = strtok(NULL, "\n");
    
    int numBytes;
    int socketSolicitante = sockets[idEquipamento-1];

    if(!equipamento_existe(idEquipamento)) {
        strcat(msg, "07,");
        strcat(msg, idSolicitante);
        strcat(msg, ",-,02");
        printf("Equipment %s not found\n", idSolicitante);
        numBytes = send(socketSolicitante, msg, strlen(msg)+1, 0);
    }
    else if(!equipamento_existe(atoi(idRequisitado))) {
        strcat(msg, "07,-,");
        strcat(msg, idRequisitado);
        strcat(msg, ",03");
        printf("Equipment %s not found\n", idRequisitado);
        numBytes = send(socketSolicitante, msg, strlen(msg)+1, 0);
    }
    else {
        // Prosseguindo com a requisição e envio de informação:
        int socketRequisitado = sockets[atoi(idRequisitado)-1];
        strcat(msg, "05,");
        strcat(msg, idSolicitante);
        strcat(msg, ",");
        strcat(msg, idRequisitado);
        strcat(msg, ",-");        
        numBytes = send(socketRequisitado, msg, strlen(msg)+1, 0);
        enviar_informacao_equipamento(buffer, idSolicitante, idRequisitado);
    }
    if(numBytes != strlen(msg)+1) {
        exibir_log_saida("send");
    }
}

void remover_equipamento(int idSolicitante) {

    int numBytes;
    // String que guardará a mensagem:
    char msg[TAMANHO_BUFFER];
    memset(msg, 0, TAMANHO_BUFFER);

    // Pegando o id do solicitante formatado:
    char *idEquipamento = malloc(sizeof(char)*TAMANHO_ID);
    sprintf(idEquipamento, "%d", idSolicitante);
    idEquipamento = formatar_id_equipamento(idEquipamento);
    int socketEquipamento = sockets[idSolicitante-1];

    if(!equipamento_existe(idSolicitante)) {
        strcat(msg, "07,-,");
        strcat(msg, idEquipamento);
        strcat(msg, ",01");
        numBytes = send(socketEquipamento, msg, strlen(msg)+1, 0);
    }
    else {
        // Prosseguindo com a remoção e notificação dessa remoção:
        strcat(msg, "08,-,");
        strcat(msg, idEquipamento);
        strcat(msg, ",01");
        printf("Equipment %s removed\n", idEquipamento);
        numEquipamentos--;
        numBytes = send(socketEquipamento, msg, strlen(msg)+1, 0);
        memset(msg, 0, TAMANHO_BUFFER);
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

bool atingiu_max_equipamentos() {
    
    return numEquipamentos == MAX_EQUIPAMENTOS;
}

int adicionar_equipamento(char buffer[TAMANHO_BUFFER], int socketEquipamento) {

    char *idEquipamento = malloc(sizeof(char)*TAMANHO_ID);
    strcpy(idEquipamento, EQUIPAMENTO_NAO_EXISTE);

    // String que guardará a mensagem:
    char msg[TAMANHO_BUFFER];
    memset(msg, 0, TAMANHO_BUFFER);


    if(atingiu_max_equipamentos())
        strcat(msg, "07,-,-,04");
    else{
        // Prosseguindo com a adição e notificação dessa adição:
        for(int posicao = 0; posicao < MAX_EQUIPAMENTOS; posicao++){    
            if(!equipamentos[posicao]) {
                equipamentos[posicao] = true;
                sockets[posicao] = socketEquipamento;
                sprintf(idEquipamento, "%d", posicao + 1);
                strcat(msg, "03,-,-,");
                strcat(msg, idEquipamento);
                idEquipamento = formatar_id_equipamento(idEquipamento);
                sprintf(buffer, "Equipment %s added", idEquipamento);
                numEquipamentos++;
                break;
            }
        }
    }

    int numBytes = send(socketEquipamento, msg, strlen(msg)+1, 0);

    if(numBytes != strlen(msg)+1)
        exibir_log_saida("send");
    if(strcmp(idEquipamento, EQUIPAMENTO_NAO_EXISTE) != 0 && numEquipamentos > 1) {
        memset(msg, 0, TAMANHO_BUFFER);
        sprintf(msg, "01,%s,-,-", idEquipamento);
        enviar_broadcast(msg, idEquipamento);
    }
    return atoi(idEquipamento);
}

void interpretar_entrada(char buffer[TAMANHO_BUFFER], int idEquipamento){
    
    char bufferAux[TAMANHO_BUFFER];
    memset(bufferAux, 0, TAMANHO_BUFFER);
    strcpy(bufferAux, buffer);
    strtok(bufferAux, " "); // Pegando o primeiro trecho da entrada para depois verificar se é um caso de requisição de informação

    if(strcmp(buffer, "list equipments\n") == 0)
        listar_equipamentos(buffer, idEquipamento);
    else if(strcmp(buffer, "close connection\n") == 0)
        remover_equipamento(idEquipamento);
    else if(strcmp(bufferAux, "request") == 0)
        requisitar_informacao_equipamento(buffer, idEquipamento);
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
    char enderecoEquipStr[TAMANHO_BUFFER];
    converter_endereco_em_string(enderecoEquip, enderecoEquipStr, TAMANHO_BUFFER);
    //printf("[log] connection from %s\n", enderecoEquipStr);

    char buffer[TAMANHO_BUFFER];
    memset(buffer, 0, TAMANHO_BUFFER);
    
    char resposta[TAMANHO_BUFFER];
    memset(resposta, 0, TAMANHO_BUFFER);
    
    idEquipamento = adicionar_equipamento(buffer, equipData->equipSocket);
    if(!idEquipamento){
       close(sockets[idEquipamento-1]);
       pthread_exit(EXIT_SUCCESS);
    }
    printf("%s\n", buffer);
    
    while(1){
        memset(buffer, 0, TAMANHO_BUFFER);
        recv(sockets[idEquipamento-1], buffer, TAMANHO_BUFFER - 1, 0);
        memset(resposta, 0, TAMANHO_BUFFER);
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
    srand((unsigned) time(NULL));
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

    char enderecoStr[TAMANHO_BUFFER];
    converter_endereco_em_string(endereco, enderecoStr, TAMANHO_BUFFER);

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

