#pragma once

#include <stdlib.h>

#include <arpa/inet.h>

void exibirLogSaida(const char *msg);

int parsearEndereco(const char *addrstr, const char *portstr, struct sockaddr_storage *storage);

void converterEnderecoEmString(const struct sockaddr *addr, char *str, size_t strsize);

int inicializarSockAddrServer(const char *portstr, struct sockaddr_storage *storage);


