#pragma once

#include <stdlib.h>

#include <arpa/inet.h>

void exibir_log_saida(const char *msg);

int analisar_endereco(const char *addrstr, const char *portstr, struct sockaddr_storage *storage);

void converter_endereco_em_string(const struct sockaddr *addr, char *str, size_t strsize);

int inicializar_sock_addr_server(const char *portstr, struct sockaddr_storage *storage);

